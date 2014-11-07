#include <asf.h>
#include "dev/watchdog.h"
#include "log.h"
#include "flash.h"


#define PAGE_SIZE   NVMCTRL_PAGE_SIZE
#define ERASE_SIZE  (PAGE_SIZE * NVMCTRL_ROW_PAGES)


static uint8_t opened;
static struct nvm_parameters parameters;

static unsigned char tmp_buffer[ERASE_SIZE];

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
}
/*---------------------------------------------------------------------------*/
static int
erase(unsigned long from, unsigned long to)
{
  enum status_code ret;
  unsigned long addr;

  from = (from / ERASE_SIZE) * ERASE_SIZE;
  to   = (to   / ERASE_SIZE) * ERASE_SIZE;

  for(addr = from; addr < to; addr += ERASE_SIZE) {
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

  /* Check if the row address is valid */
  if ((addr + len) >
      ((uint32_t)parameters.page_size * parameters.nvm_number_of_pages)) {
    WARN("addr is outside flash region");
    return -1;
  }

  while (len > 0) {
    /* Determine start address of page */
    page_addr = (addr / ERASE_SIZE) * ERASE_SIZE;

    /* Copy content of current page to tmp_buffer */
    memcpy(tmp_buffer, (void *)page_addr, ERASE_SIZE);

    /* Wait until flash is ready */
    while (!nvm_is_ready());

    /* Erase current page */
    nvm_erase_row(page_addr);

    /* Determine number of bytes to copy */
    copy_len = min(len, (page_addr + ERASE_SIZE) - addr);

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
const struct flash_driver samr21_flash =
{
  open,
  close,
  erase,
  read,
  write
};
