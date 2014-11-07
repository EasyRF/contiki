#include <asf.h>
#include "contiki.h"
#include "esd_spi_master.h"
#include "log.h"


static struct spi_slave_inst slave;


void
enc28j60_arch_spi_init(void)
{
  struct spi_slave_inst_config slave_dev_config;

  /* Init SPI master interface */
  esd_spi_master_init();

  /* Congfigure slave SPI device */
  spi_slave_inst_get_config_defaults(&slave_dev_config);
  slave_dev_config.ss_pin = ETHERNET_CS;
  spi_attach_slave(&slave, &slave_dev_config);
}
/*---------------------------------------------------------------------------*/
uint8_t
enc28j60_arch_spi_write(uint8_t data)
{
  uint16_t in;
  spi_transceive_wait(&spi_master_instance, data, &in);
  return in & 0xff;
}
/*---------------------------------------------------------------------------*/
uint8_t
enc28j60_arch_spi_read(void)
{
  uint16_t out = 0xff, in = 0;
  spi_transceive_wait(&spi_master_instance, out, &in);
  return in & 0xff;
}
/*---------------------------------------------------------------------------*/
void
enc28j60_arch_spi_select(void)
{
  spi_select_slave(&spi_master_instance, &slave, true);
}
/*---------------------------------------------------------------------------*/
void
enc28j60_arch_spi_deselect(void)
{
  spi_select_slave(&spi_master_instance, &slave, false);
}
/*---------------------------------------------------------------------------*/
