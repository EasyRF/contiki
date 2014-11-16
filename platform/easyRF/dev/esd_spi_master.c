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
#include "log.h"
#include "esd_spi_master.h"


static bool initialized = false;

struct spi_module esd_spi_master_instance;

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
    spi_init(&esd_spi_master_instance, ESD_SPI_MODULE, &config_spi_master);
    spi_enable(&esd_spi_master_instance);

    INFO("ESD SPI master interface initialized");

    initialized = true;
  }
}
/*---------------------------------------------------------------------------*/
