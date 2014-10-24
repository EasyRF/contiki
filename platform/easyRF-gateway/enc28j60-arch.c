#include <asf.h>
#include "contiki.h"
#include "enc28j60-arch.h"
#include "log.h"

#undef TRACE
#define TRACE(...)

struct spi_module spi_master_instance;
struct spi_slave_inst slave;


void
enc28j60_arch_spi_init(void)
{
  struct spi_config config_spi_master;
  struct spi_slave_inst_config slave_dev_config;

  spi_slave_inst_get_config_defaults(&slave_dev_config);
  slave_dev_config.ss_pin = ETHERNET_CS;
  spi_attach_slave(&slave, &slave_dev_config);

  spi_get_config_defaults(&config_spi_master);
  config_spi_master.mode_specific.master.baudrate = ESD_SPI_BAUDRATE;
  config_spi_master.mux_setting = ESD_SPI_SERCOM_MUX_SETTING;
  config_spi_master.pinmux_pad0 = ESD_SPI_SERCOM_PINMUX_PAD0;
  config_spi_master.pinmux_pad1 = ESD_SPI_SERCOM_PINMUX_PAD1;
  config_spi_master.pinmux_pad2 = ESD_SPI_SERCOM_PINMUX_PAD2;
  config_spi_master.pinmux_pad3 = ESD_SPI_SERCOM_PINMUX_PAD3;
  spi_init(&spi_master_instance, ESD_SPI_MODULE, &config_spi_master);
  spi_enable(&spi_master_instance);
}
/*---------------------------------------------------------------------------*/
uint8_t
enc28j60_arch_spi_write(uint8_t data)
{
  uint16_t in;
  TRACE("write: %01X", data);
  spi_transceive_wait(&spi_master_instance, data, &in);
  return in & 0xff;
}
/*---------------------------------------------------------------------------*/
uint8_t
enc28j60_arch_spi_read(void)
{
  uint16_t out = 0xff, in = 0;
  spi_transceive_wait(&spi_master_instance, out, &in);
  TRACE("read: %01X", in & 0xff);
  return in & 0xff;
}
/*---------------------------------------------------------------------------*/
void
enc28j60_arch_spi_select(void)
{
  TRACE("select");
  spi_select_slave(&spi_master_instance, &slave, true);
}
/*---------------------------------------------------------------------------*/
void
enc28j60_arch_spi_deselect(void)
{
  TRACE("deselect");
  spi_select_slave(&spi_master_instance, &slave, false);
}
