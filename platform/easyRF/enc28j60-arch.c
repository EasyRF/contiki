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
#include "enc28j60.h"
#include "log.h"


#define ENC28J60_IRQ_PIN              PIN_PA23
#define ENC28J60_IRQ_PINMUX           PINMUX_PA23A_EIC_EXTINT7
#define ENC28J60_IRQ_CHANNEL          7


/* Keep the configurations static to be able to apply them quickly */
static struct extint_chan_conf enc28j60_extint_conf;
static struct spi_slave_inst_config slave_dev_config;
static struct spi_slave_inst slave;

/*---------------------------------------------------------------------------*/
static void
extint_handler(void)
{
  /* Clear the extint interrupt */
  extint_chan_clear_detected(ENC28J60_IRQ_CHANNEL);

  /* Call the interrupt handler */
  enc28j60_interrupt();
}
/*---------------------------------------------------------------------------*/
static void
configure_cs(bool as_output)
{
  if (as_output) {
    /* Disable the interrupt handler */
    extint_chan_disable_callback(ENC28J60_IRQ_CHANNEL, EXTINT_CALLBACK_TYPE_DETECT);

    /* Apply SPI slave configuration */
    spi_attach_slave(&slave, &slave_dev_config);
  } else {
    /* Apply extint configuration */
    extint_chan_set_config(ENC28J60_IRQ_CHANNEL, &enc28j60_extint_conf);

    /* Enable the interrupt handler */
    extint_chan_enable_callback(ENC28J60_IRQ_CHANNEL, EXTINT_CALLBACK_TYPE_DETECT);
  }
}
/*---------------------------------------------------------------------------*/
void
enc28j60_arch_spi_init(void)
{
  /* Init SPI master interface */
  esd_spi_master_init();

  /* Setup extint configuration for receiving interrupts from the enc28j60 */
  extint_chan_get_config_defaults(&enc28j60_extint_conf);
  enc28j60_extint_conf.gpio_pin = ENC28J60_IRQ_PIN;
  enc28j60_extint_conf.gpio_pin_mux = ENC28J60_IRQ_PINMUX;
  enc28j60_extint_conf.gpio_pin_pull = EXTINT_PULL_UP;
  enc28j60_extint_conf.detection_criteria = EXTINT_DETECT_FALLING;

  /* Congfigure slave SPI device */
  spi_slave_inst_get_config_defaults(&slave_dev_config);
  slave_dev_config.ss_pin = ETHERNET_CS;

  /* Register the interrupt handler */
  extint_register_callback(extint_handler, ENC28J60_IRQ_CHANNEL, EXTINT_CALLBACK_TYPE_DETECT);
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
  configure_cs(true);
  spi_select_slave(&spi_master_instance, &slave, true);
}
/*---------------------------------------------------------------------------*/
void
enc28j60_arch_spi_deselect(void)
{
  spi_select_slave(&spi_master_instance, &slave, false);
  configure_cs(false);
}
/*---------------------------------------------------------------------------*/
