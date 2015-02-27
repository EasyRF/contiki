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
#include "dev/watchdog.h"
#include "log.h"
#include "flash.h"


#undef TRACE
#define TRACE(...)


#define PAGE_SIZE       NVMCTRL_PAGE_SIZE
#define SECTOR_SIZE     (PAGE_SIZE * NVMCTRL_ROW_PAGES)
#define SECTOR_COUNT    (NVMCTRL_FLASH_SIZE / SECTOR_SIZE)


static uint8_t opened;
static struct nvm_parameters parameters;

static unsigned char tmp_buffer[SECTOR_SIZE];

/*---------------------------------------------------------------------------*/
static int
open(void)
{
  if (!opened) {
    struct nvm_config config_nvm;
    nvm_get_config_defaults(&config_nvm);
    nvm_set_config(&config_nvm);
    nvm_get_parameters(&parameters);

    TRACE("bootloader_number_of_pages: %ld, eeprom_number_of_pages: %ld, nvm_number_of_pages: %d, page_size: %d",
          parameters.bootloader_number_of_pages,
          parameters.eeprom_number_of_pages,
          parameters.nvm_number_of_pages,
          parameters.page_size);

    while (!nvm_is_ready());

    opened = 1;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static void
close(void)
{
  /* Do nothing */
  opened = 0;
}
/*---------------------------------------------------------------------------*/
static int
erase(unsigned long from, unsigned long to)
{
  enum status_code ret;
  unsigned long addr;

  if (!opened) {
    return -1;
  }

  from = (from / SECTOR_SIZE) * SECTOR_SIZE;
  to   = (to   / SECTOR_SIZE) * SECTOR_SIZE;

  for(addr = from; addr < to; addr += SECTOR_SIZE) {
    TRACE("addr: %ld", addr);
    if ((ret = nvm_erase_row(addr)) != STATUS_OK) {
      WARN("nvm_erase_row error: %d", ret);
      return -1;
    }
    watchdog_periodic();
  }

  while (!nvm_is_ready());

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
read(unsigned long addr, unsigned char * buffer, unsigned long len)
{
  if (!opened) {
    return -1;
  }

  /* Check if the row address is valid */
  if ((addr + len) >
      ((uint32_t)parameters.page_size * parameters.nvm_number_of_pages)) {
    WARN("addr is outside flash region");
    return -1;
  }

  /* Flash of samr21 is memory mapped so just do a memcpy */
  memcpy(buffer, (void *)addr, len);

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
write(unsigned long addr, const unsigned char * buffer, unsigned long len)
{
  enum status_code ret;
  unsigned long page_addr;
  int copy_len, i;

  if (!opened) {
    return -1;
  }

  /* Check if the row address is valid */
  if ((addr + len) >
      ((uint32_t)parameters.page_size * parameters.nvm_number_of_pages)) {
    WARN("addr is outside flash region");
    return -1;
  }

  while (len > 0) {
    /* Determine start address of page */
    page_addr = (addr / SECTOR_SIZE) * SECTOR_SIZE;

    /* Copy content of current page to tmp_buffer */
    memcpy(tmp_buffer, (void *)page_addr, SECTOR_SIZE);

    /* Wait until flash is ready */
    while (!nvm_is_ready());

    /* Erase current page */
    nvm_erase_row(page_addr);

    /* Determine number of bytes to copy */
    copy_len = min(len, (page_addr + SECTOR_SIZE) - addr);

    /* Update tmp_buffer with content from buffer */
    memcpy(&tmp_buffer[addr - page_addr], buffer, copy_len);

    /* Update buffer, len and addr */
    len    -= copy_len;
    addr   += copy_len;
    buffer += copy_len;

    /* Wait until flash is ready */
    while (!nvm_is_ready());

    /* Write contents of tmp_buffer to flash in NVMCTRL_ROW_PAGES steps */
    for (i = 0; i < NVMCTRL_ROW_PAGES; i++) {
      if ((ret = nvm_write_buffer(page_addr + i * PAGE_SIZE, &tmp_buffer[i * PAGE_SIZE], PAGE_SIZE)) != STATUS_OK) {
        WARN("nvm_write_buffer error: %d", ret);
        return -1;
      }
    }
  }

  /* Return OK */
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
sector_size(void)
{
  if (!opened) {
    return -1;
  }

  return SECTOR_SIZE;
}
/*---------------------------------------------------------------------------*/
static int
sector_count(void)
{
  if (!opened) {
    return -1;
  }

  return SECTOR_COUNT;
}
/*---------------------------------------------------------------------------*/
static int
page_size(void)
{
  if (!opened) {
    return -1;
  }

  return PAGE_SIZE;
}
/*---------------------------------------------------------------------------*/
const struct flash_driver samr21_flash =
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
