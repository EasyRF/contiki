#include <asf.h>
#include "conf_at25dfx.h"

#include "dev/watchdog.h"
#include "log.h"
#include "flash.h"


static struct spi_module at25dfx_spi;
static struct at25dfx_chip_module at25dfx_chip;
static uint8_t opened;

/*---------------------------------------------------------------------------*/
static int
open(void)
{
  if (!opened) {
    struct at25dfx_chip_config at25dfx_chip_config;
    struct spi_config at25dfx_spi_config;

    at25dfx_spi_get_config_defaults(&at25dfx_spi_config);
    at25dfx_spi_config.mode_specific.master.baudrate = AT25DFX_CLOCK_SPEED;
    at25dfx_spi_config.mux_setting = AT25DFX_SPI_PINMUX_SETTING;
    at25dfx_spi_config.pinmux_pad0 = AT25DFX_SPI_PINMUX_PAD0;
    at25dfx_spi_config.pinmux_pad1 = AT25DFX_SPI_PINMUX_PAD1;
    at25dfx_spi_config.pinmux_pad2 = AT25DFX_SPI_PINMUX_PAD2;
    at25dfx_spi_config.pinmux_pad3 = AT25DFX_SPI_PINMUX_PAD3;

    spi_init(&at25dfx_spi, AT25DFX_SPI, &at25dfx_spi_config);
    spi_enable(&at25dfx_spi);

    at25dfx_chip_config.type = AT25DFX_MEM_TYPE;
    at25dfx_chip_config.cs_pin = AT25DFX_CS;

    at25dfx_chip_init(&at25dfx_chip, &at25dfx_spi, &at25dfx_chip_config);

    at25dfx_chip_wake(&at25dfx_chip);

    if (at25dfx_chip_check_presence(&at25dfx_chip) != STATUS_OK) {
      WARN("at25dfx not found");
      return -1;
    }

    INFO("at25dfx initialized");

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

  if ((ret = at25dfx_chip_erase_block(&at25dfx_chip, from, to - from)) != STATUS_OK) {
    WARN("at25dfx_chip_write_buffer error: %d", ret);
    return -1;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
read(unsigned long addr, unsigned char * buffer, unsigned long len)
{
  enum status_code ret;

  if ((ret = at25dfx_chip_read_buffer(&at25dfx_chip, addr, buffer, len)) != STATUS_OK) {
    WARN("at25dfx_chip_read_buffer error: %d", ret);
    return -1;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
write(unsigned long addr, const unsigned char * buffer, unsigned long len)
{
  enum status_code ret;

  if ((ret = at25dfx_chip_write_buffer(&at25dfx_chip, addr, buffer, len)) != STATUS_OK) {
    WARN("at25dfx_chip_write_buffer error: %d", ret);
    return -1;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
const struct flash_driver at25df32s_flash =
{
  open,
  close,
  erase,
  read,
  write
};
