#include <asf.h>
#include "esd_spi_master.h"
#include "dev/watchdog.h"
#include "log.h"
#include "flash_sst25vf032b.h"

/* Bitshift helper */
#define BM(pos)         ((uint32_t)1 << pos)

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
  sst25vf032b_arch_spi_write(((addr & 0xFFFFFF) >> 16));
  sst25vf032b_arch_spi_write(((addr & 0xFFFF) >> 8));
  sst25vf032b_arch_spi_write(addr & 0xFF);
}
/*---------------------------------------------------------------------------*/
static uint8_t
read_status_register(void)
{
  uint8_t status = 0;

  sst25vf032b_arch_spi_select();
  sst25vf032b_arch_spi_write(READ_STATUS_REG);
  status = sst25vf032b_arch_spi_read();
  sst25vf032b_arch_spi_deselect();

//  TRACE("status = %02X", status);

  return status;
}
/*---------------------------------------------------------------------------*/
static inline void
wait_busy(void)
{
  uint8_t status;
  do {
    status = read_status_register();
  } while ((status & STATUS_BUSY) == STATUS_BUSY);
}
/*---------------------------------------------------------------------------*/
static uint32_t
read_jedec_id(void)
{
  unsigned long jedec_id = 0;

  sst25vf032b_arch_spi_select();
  sst25vf032b_arch_spi_write(READ_JEDEC_ID);
  jedec_id = (jedec_id | sst25vf032b_arch_spi_read()) << 8;
  jedec_id = (jedec_id | sst25vf032b_arch_spi_read()) << 8;
  jedec_id = (jedec_id | sst25vf032b_arch_spi_read());
  sst25vf032b_arch_spi_deselect();

  TRACE("JEDEC ID: %ld", jedec_id);

  return jedec_id;
}
/*---------------------------------------------------------------------------*/
static void
write_enable(void)
{
  do {
    sst25vf032b_arch_spi_select();
    sst25vf032b_arch_spi_write(WRITE_ENABLE_CMD);
    sst25vf032b_arch_spi_deselect();
  } while ((read_status_register() & STATUS_WEL) != STATUS_WEL);
  TRACE("Write enabled");
}
/*---------------------------------------------------------------------------*/
static void
write_disable(void)
{
  do {
    sst25vf032b_arch_spi_select();
    sst25vf032b_arch_spi_write(WRITE_DISABLE_CMD);
    sst25vf032b_arch_spi_deselect();
  } while ((read_status_register() & STATUS_WEL) == STATUS_WEL);
  TRACE("Write disabled");
}
/*---------------------------------------------------------------------------*/
static void
write_status_register(uint8_t status)
{
  write_enable();
  sst25vf032b_arch_spi_select();
  sst25vf032b_arch_spi_write(WRITE_STAT_REG);
  sst25vf032b_arch_spi_write(status);
  sst25vf032b_arch_spi_deselect();
}
/*---------------------------------------------------------------------------*/
static void
chip_erase(void)
{
  write_enable();
  sst25vf032b_arch_spi_select();
  sst25vf032b_arch_spi_write(CHIP_ERASE_CMD);
  sst25vf032b_arch_spi_deselect();
  wait_busy();
  TRACE("Full chip erase completed");
}
/*---------------------------------------------------------------------------*/
static void
block_erase(unsigned long addr, uint8_t erase_cmd)
{
  write_enable();
  sst25vf032b_arch_spi_select();
  sst25vf032b_arch_spi_write(erase_cmd);
  send_address(addr);
  sst25vf032b_arch_spi_deselect();
  wait_busy();
  TRACE("Block erase completed");
}
/*---------------------------------------------------------------------------*/
static int
open(void)
{
  if (!opened) {

    /* Initalize SPI interface */
    sst25vf032b_arch_spi_init();

    /* Read ID */
    if (read_jedec_id() != 0xBF254A) {
      WARN("Invalid chip ID");
      return -1;
    }

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
}
/*---------------------------------------------------------------------------*/
static int
erase(unsigned long from, unsigned long to)
{
  unsigned long addr;

  /* Full chip erase requested */
  if (from == 0 && to == SST25VF032B_SIZE) {
    chip_erase();
    return 1;
  }

  /* Range erase requested.
   * Try to erase using largest block sizes
   */
  for (addr = from; addr < to;) {
    if ((to - addr) >= BLOCK_SIZE_64K) {
      block_erase(addr, BLOCK_ERASE_64K_CMD);
      addr += BLOCK_SIZE_64K;
    } else if ((to - addr) >= BLOCK_SIZE_32K) {
      block_erase(addr, BLOCK_ERASE_32K_CMD);
      addr += BLOCK_SIZE_32K;
    } else {
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

  sst25vf032b_arch_spi_select();
  sst25vf032b_arch_spi_write(READ_CMD);
  send_address(addr);
  for (i = 0; i < len; i++) {
    buffer[i] = sst25vf032b_arch_spi_read();
  }
  sst25vf032b_arch_spi_deselect();

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
write(unsigned long addr, const unsigned char * buffer, unsigned long len)
{
  unsigned long start;
  const unsigned char * ptr;

  /* Erase area before writing */
  erase(addr, addr + len);

  ptr = buffer;

  /* Write just 1 byte */
  if (len == 1) {
    write_enable();
    sst25vf032b_arch_spi_select();
    sst25vf032b_arch_spi_write(BYTE_PROGRAM_CMD);
    send_address(addr);
    sst25vf032b_arch_spi_write(*ptr);
    sst25vf032b_arch_spi_deselect();
    wait_busy();
    TRACE("Wrote 1 byte");
    return 1;
  }

  /* Write 2 bytes */
  write_enable();
  sst25vf032b_arch_spi_select();
  sst25vf032b_arch_spi_write(AAI_WORD_PROGRAM_CMD);
  send_address(addr);
  sst25vf032b_arch_spi_write(*ptr++);
  sst25vf032b_arch_spi_write(*ptr++);
  sst25vf032b_arch_spi_deselect();
  wait_busy();

  for (start = addr + 2; start < (addr + len); start += 2) {
    sst25vf032b_arch_spi_select();
    sst25vf032b_arch_spi_write(AAI_WORD_PROGRAM_CMD);
    sst25vf032b_arch_spi_write(*ptr++);
    sst25vf032b_arch_spi_write(*ptr++);
    sst25vf032b_arch_spi_deselect();
    wait_busy();
  }

  write_disable();

  TRACE("Wrote %ld bytes", len);

  return 1;
}
/*---------------------------------------------------------------------------*/
const struct flash_driver flash_sst25vf032b =
{
  open,
  close,
  erase,
  read,
  write
};
