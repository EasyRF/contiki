#include <asf.h>
#include "contiki.h"
#include "log.h"
#include "esd_spi_master.h"


static bool initialized = false;

struct spi_module spi_master_instance;

/*---------------------------------------------------------------------------*/
void
esd_spi_master_init(void)
{
  if (!initialized) {
    struct spi_config config_spi_master;
    spi_get_config_defaults(&config_spi_master);
    config_spi_master.mode_specific.master.baudrate = ESD_SPI_BAUDRATE;
    config_spi_master.mux_setting = ESD_SPI_SERCOM_MUX_SETTING;
    config_spi_master.pinmux_pad0 = ESD_SPI_SERCOM_PINMUX_PAD0;
    config_spi_master.pinmux_pad1 = ESD_SPI_SERCOM_PINMUX_PAD1;
    config_spi_master.pinmux_pad2 = ESD_SPI_SERCOM_PINMUX_PAD2;
    config_spi_master.pinmux_pad3 = ESD_SPI_SERCOM_PINMUX_PAD3;
    spi_init(&spi_master_instance, ESD_SPI_MODULE, &config_spi_master);
    spi_enable(&spi_master_instance);

    INFO("ESD SPI master interface initialized");

    initialized = true;
  }
}
/*---------------------------------------------------------------------------*/
