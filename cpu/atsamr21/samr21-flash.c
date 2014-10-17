#include <asf.h>
#include "dev/watchdog.h"
#include "log.h"
#include "flash.h"


static uint8_t opened;
static struct nvm_parameters parameters;

/*---------------------------------------------------------------------------*/
void open(void)
{
  if (!opened) {
    struct nvm_config config_nvm;
    nvm_get_config_defaults(&config_nvm);
    nvm_set_config(&config_nvm);
    nvm_get_parameters(&parameters);
  }
}
/*---------------------------------------------------------------------------*/
void close(void)
{
  /* Not implemented */
}
/*---------------------------------------------------------------------------*/
int erase(unsigned long from, unsigned long to)
{
  enum status_code ret;
  unsigned long addr;

  for(addr = from; addr < to; addr += parameters.page_size) {
    if ((ret = nvm_erase_row(addr)) != STATUS_OK) {
      WARN("nvm_erase_row error: %d", ret);
      return -1;
    }
    watchdog_periodic();
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
int read(unsigned long addr, unsigned char * buffer, unsigned long len)
{
  enum status_code ret;

  if ((ret = nvm_read_buffer(addr, buffer, len)) != STATUS_OK) {
    WARN("nvm_read_buffer error: %d", ret);
    return -1;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
int write(unsigned long addr, const unsigned char * buffer, unsigned long len)
{
  enum status_code ret;

  if ((ret = nvm_write_buffer(addr, buffer, len)) != STATUS_OK) {
    WARN("nvm_write_buffer error: %d", ret);
    return -1;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
const struct flash_driver samr21_flash_driver =
{
  open,
  close,
  erase,
  read,
  write
};
