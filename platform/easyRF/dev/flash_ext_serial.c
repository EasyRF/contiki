/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 EasyRF
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <asf.h>
#include "esd_spi_master.h"
#include "dev/watchdog.h"
#include "log.h"
#include "flash_ext_serial.h"

#undef TRACE
#define TRACE(...)


/* Supported flash devices */
enum flash_device_type {
  SST25VF032B,
  S25FL132K,
  LAST_CHIP
};

/* Properties of flash devices */
struct flash_device {
  enum flash_device_type type;
  char name[16];
  uint32_t jedec_id;
  int sector_size;
  int sector_count;
  int page_size;
};

static struct flash_device flash_devices[] = {
  { SST25VF032B, "SST25VF032B", 0x00BF254A, 4096, 1024,  -1 },
  { S25FL132K,   "S25FL132K",   0x00014016, 4096, 1024, 256 }
};

static struct flash_device * flash_device;

/* Memory sizes */
#define SECTOR_SIZE           4096
#define SECTOR_COUNT          1024
#define PAGE_SIZE             256

/* Bitshift helper */
#define BM(pos)               ((uint32_t)1 << pos)

/* Commands */
#define READ_JEDEC_ID         0x9F
#define READ_STATUS_REG       0x05
#define ENABLE_WRITE_STAT_REG 0x50
#define WRITE_ENABLE_CMD      0x06
#define WRITE_DISABLE_CMD     0x04
#define WRITE_STAT_REG        0x01
#define READ_CMD              0x03
#define READ_HS_CMD           0x0B
#define BYTE_PROGRAM_CMD      0x02
#define AAI_WORD_PROGRAM_CMD  0xAD
#define CHIP_ERASE_CMD        0x60
#define BLOCK_ERASE_4K_CMD    0x20
#define BLOCK_ERASE_32K_CMD   0x52
#define BLOCK_ERASE_64K_CMD   0xD8

/* Status bits */
#define STATUS_BUSY           BM(0) /* Erase or write in progress */
#define STATUS_WEL            BM(1) /* Write enabled */
#define STATUS_BP0            BM(2) /* Block level write protection */
#define STATUS_BP1            BM(3) /* Block level write protection */
#define STATUS_BP2            BM(4) /* Block level write protection */
#define STATUS_BP3            BM(5) /* Block level write protection */
#define STATUS_AAI            BM(6) /* Auto Address Incrrement enabled */
#define STATUS_BPL            BM(7) /* Block level bits read-only */

/* Block erase sizes */
#define BLOCK_SIZE_4K         ( 4 * 1024)
#define BLOCK_SIZE_32K        (32 * 1024)
#define BLOCK_SIZE_64K        (64 * 1024)


static bool opened;

/*---------------------------------------------------------------------------*/
static inline void
send_address(unsigned long addr)
{
  flash_ext_serial_arch_spi_write(((addr & 0xFFFFFF) >> 16));
  flash_ext_serial_arch_spi_write(((addr & 0xFFFF) >> 8));
  flash_ext_serial_arch_spi_write(addr & 0xFF);
}
/*---------------------------------------------------------------------------*/
static uint8_t
read_status_register(void)
{
  uint8_t status = 0;

  flash_ext_serial_arch_spi_select();
  flash_ext_serial_arch_spi_write(READ_STATUS_REG);
  status = flash_ext_serial_arch_spi_read();
  flash_ext_serial_arch_spi_deselect();

//  TRACE("status = %02X", status);

  return status;
}
/*---------------------------------------------------------------------------*/
static inline void
wait_busy(void)
{
  uint32_t i = 0;
  uint8_t status;
  do {
    status = read_status_register();
    if (++i % 1000 == 0) {
      watchdog_periodic();
      TRACE("status = %02X", status);
    }
  } while ((status & STATUS_BUSY) == STATUS_BUSY);
}
/*---------------------------------------------------------------------------*/
static uint32_t
read_jedec_id(void)
{
  unsigned long jedec_id = 0;

  flash_ext_serial_arch_spi_select();
  flash_ext_serial_arch_spi_write(READ_JEDEC_ID);
  jedec_id = (jedec_id | flash_ext_serial_arch_spi_read()) << 8;
  jedec_id = (jedec_id | flash_ext_serial_arch_spi_read()) << 8;
  jedec_id = (jedec_id | flash_ext_serial_arch_spi_read());
  flash_ext_serial_arch_spi_deselect();

  TRACE("JEDEC ID: %04lX", jedec_id);

  return jedec_id;
}
/*---------------------------------------------------------------------------*/
static void
write_enable(void)
{
  do {
    flash_ext_serial_arch_spi_select();
    flash_ext_serial_arch_spi_write(WRITE_ENABLE_CMD);
    flash_ext_serial_arch_spi_deselect();
  } while ((read_status_register() & STATUS_WEL) != STATUS_WEL);
  TRACE("Write enabled");
}
/*---------------------------------------------------------------------------*/
static void
write_disable(void)
{
  do {
    flash_ext_serial_arch_spi_select();
    flash_ext_serial_arch_spi_write(WRITE_DISABLE_CMD);
    flash_ext_serial_arch_spi_deselect();
  } while ((read_status_register() & STATUS_WEL) == STATUS_WEL);
  TRACE("Write disabled");
}
/*---------------------------------------------------------------------------*/
static void
write_status_register(uint8_t status)
{
  write_enable();
  flash_ext_serial_arch_spi_select();
  flash_ext_serial_arch_spi_write(WRITE_STAT_REG);
  flash_ext_serial_arch_spi_write(status);
  flash_ext_serial_arch_spi_deselect();
}
/*---------------------------------------------------------------------------*/
static void
chip_erase(void)
{
  TRACE("Starting full chip erase");
  write_enable();
  flash_ext_serial_arch_spi_select();
  flash_ext_serial_arch_spi_write(CHIP_ERASE_CMD);
  flash_ext_serial_arch_spi_deselect();
  TRACE("Wait for erase complete");
  wait_busy();
  TRACE("Full chip erase completed");
}
/*---------------------------------------------------------------------------*/
static void
block_erase(unsigned long addr, uint8_t erase_cmd)
{
  write_enable();
  flash_ext_serial_arch_spi_select();
  flash_ext_serial_arch_spi_write(erase_cmd);
  send_address(addr);
  flash_ext_serial_arch_spi_deselect();
  wait_busy();
  TRACE("Block erase completed");
}
/*---------------------------------------------------------------------------*/
static int
open(void)
{
  if (!opened) {

    /* Initalize SPI interface */
    flash_ext_serial_arch_spi_init();

    /* Reset device */
    write_disable();

    /* Read ID */
    uint32_t id = read_jedec_id();
    flash_device = 0;
    for (int i = 0; i < LAST_CHIP; i++) {
      if (id == flash_devices[i].jedec_id) {
        flash_device = &flash_devices[i];
      }
    }

    if (!flash_device) {
      WARN("No supported flash device found");
      return -1;
    }

    INFO("Found flash device: %s", flash_device->name);

    /* Unprotect all block levels */
    write_status_register(0x00);

    /* Update state */
    opened = true;

    /* Log */
    INFO("serial flash initialized");
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static void
close(void)
{
  opened = 0;
}
/*---------------------------------------------------------------------------*/
static int
erase(unsigned long from, unsigned long to)
{
  unsigned long addr;

  if (!opened) {
    return -1;
  }

  TRACE("Erasing from address %ld to address %ld", from, to);

  /* Full chip erase requested */
  if (from == 0 && to == (SECTOR_SIZE * SECTOR_COUNT)) {
    chip_erase();
    return 1;
  }

  /* Range erase requested.
   * Try to erase using largest block sizes
   */
  for (addr = from; addr < to;) {
    if ((to - addr) >= BLOCK_SIZE_64K) {
      TRACE("64K block erase");
      block_erase(addr, BLOCK_ERASE_64K_CMD);
      addr += BLOCK_SIZE_64K;
    } else if ((to - addr) >= BLOCK_SIZE_32K &&
               flash_device->type == SST25VF032B) {
      block_erase(addr, BLOCK_ERASE_32K_CMD);
      addr += BLOCK_SIZE_32K;
    } else {
      TRACE("4K block erase");
      block_erase(addr, BLOCK_ERASE_4K_CMD);
      addr += BLOCK_SIZE_4K;
    }
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
read(unsigned long addr, unsigned char * buffer, unsigned long len)
{
  unsigned long i;

  if (!opened) {
    return -1;
  }

  TRACE("Read %ld bytes from address %ld", len, addr);

  flash_ext_serial_arch_spi_select();
  flash_ext_serial_arch_spi_write(READ_CMD);
  send_address(addr);
  for (i = 0; i < len; i++) {
    buffer[i] = flash_ext_serial_arch_spi_read();
  }
  flash_ext_serial_arch_spi_deselect();

  return 1;
}
/*---------------------------------------------------------------------------*/
static void
write_byte(unsigned long addr, const unsigned char byte)
{
  write_enable();
  flash_ext_serial_arch_spi_select();
  flash_ext_serial_arch_spi_write(BYTE_PROGRAM_CMD);
  send_address(addr);
  flash_ext_serial_arch_spi_write(byte);
  flash_ext_serial_arch_spi_deselect();
  wait_busy();
  write_disable();
  TRACE("Wrote 1 byte to %ld", addr);
}
/*---------------------------------------------------------------------------*/
static int
write(unsigned long from, const unsigned char * buffer, unsigned long len)
{
  unsigned long addr;
  const unsigned char * ptr;

  if (!opened) {
    return -1;
  }

  TRACE("Writing %ld bytes to address %ld", len, from);

  ptr = buffer;
  addr = from;

  if (flash_device->type == SST25VF032B) {
    unsigned long remaining = len;

    /* Write single byte when length is 1 or address is odd */
    if ((remaining == 1) || (addr & 1)) {
      write_byte(addr++, *ptr++);
      remaining--;
      TRACE("First byte written");
    }

    /* Need to send the destination address once */
    int first_time = 1;

    /* Keep writing two bytes at a time */
    while (remaining > 1) {
      if (first_time) {
        write_enable();
      }
      flash_ext_serial_arch_spi_select();
      flash_ext_serial_arch_spi_write(AAI_WORD_PROGRAM_CMD);
      if (first_time) {
        send_address(addr);
        first_time = 0;
      }
      flash_ext_serial_arch_spi_write(*ptr++);
      flash_ext_serial_arch_spi_write(*ptr++);
      flash_ext_serial_arch_spi_deselect();
      wait_busy();
      remaining -= 2;
      addr += 2;
      TRACE("2 bytes written");
    }
    write_disable();

    if (remaining == 1) {
      write_byte(addr, *ptr);
      TRACE("Last byte written");
    }

  } else if (flash_device->type == S25FL132K) {

    /* Initialize to non-existing page */
    int32_t page = -1;

    for (addr = from; addr < (from + len); addr++) {
      /* Calculate new page for address */
      int new_page = addr / flash_device->page_size;
      /* Check if we're still on the same page ;) */
      if (page != new_page) {
        /* Remember new page */
        page = new_page;

        /* Finish programming previous page */
        flash_ext_serial_arch_spi_deselect();
        wait_busy();

        /* Prepare chip for programming by sending address */
        write_enable();
        flash_ext_serial_arch_spi_select();
        flash_ext_serial_arch_spi_write(BYTE_PROGRAM_CMD);
        send_address(addr);
      }
      /* Write one byte */
      flash_ext_serial_arch_spi_write(*ptr++);
    }

    /* Finish programming */
    flash_ext_serial_arch_spi_deselect();
    wait_busy();
  }

  TRACE("Wrote %ld bytes", len);

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
sector_size(void)
{
  if (!opened) {
    return -1;
  }

  return flash_device->sector_size;
}
/*---------------------------------------------------------------------------*/
static int
sector_count(void)
{
  if (!opened) {
    return -1;
  }

  return flash_device->sector_count;
}
/*---------------------------------------------------------------------------*/
static int
page_size(void)
{
  if (!opened) {
    return -1;
  }

  return flash_device->page_size;
}
/*---------------------------------------------------------------------------*/
const struct flash_driver flash_ext_serial =
{
  open,
  close,
  erase,
  read,
  write,
  sector_size,
  sector_count,
  page_size
};
