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
#include "contiki.h"
#include "esd_spi_master.h"
#include "log.h"


static struct spi_slave_inst sst25vf032b_spi_slave;


void
sst25vf032b_arch_spi_init(void)
{
  struct spi_slave_inst_config slave_dev_config;

  /* Init SPI master interface */
  esd_spi_master_init();

  /* Congfigure slave SPI device */
  spi_slave_inst_get_config_defaults(&slave_dev_config);
  slave_dev_config.ss_pin = SERIAL_FLASH_CS;
  spi_attach_slave(&sst25vf032b_spi_slave, &slave_dev_config);
}
/*---------------------------------------------------------------------------*/
uint8_t
sst25vf032b_arch_spi_write(uint8_t data)
{
  uint16_t in;
  spi_transceive_wait(&esd_spi_master_instance, data, &in);
  return in & 0xff;
}
/*---------------------------------------------------------------------------*/
uint8_t
sst25vf032b_arch_spi_read(void)
{
  uint16_t out = 0xff, in = 0;
  spi_transceive_wait(&esd_spi_master_instance, out, &in);
  return in & 0xff;
}
/*---------------------------------------------------------------------------*/
void
sst25vf032b_arch_spi_select(void)
{
  spi_select_slave(&esd_spi_master_instance, &sst25vf032b_spi_slave, true);
}
/*---------------------------------------------------------------------------*/
void
sst25vf032b_arch_spi_deselect(void)
{
  spi_select_slave(&esd_spi_master_instance, &sst25vf032b_spi_slave, false);
}
/*---------------------------------------------------------------------------*/

