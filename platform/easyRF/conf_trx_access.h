/**
 * \file *********************************************************************
 *
 * \brief Common TRX Access Configuration
 *
 * Copyright (c) 2013-2014 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 */

#ifndef CONF_TRX_ACCESS_H_INCLUDED
#define CONF_TRX_ACCESS_H_INCLUDED

#include <parts.h>
#include "board.h"

#ifndef AT86RFX_SPI_BAUDRATE
#define AT86RFX_SPI_BAUDRATE         (4000000)
#endif

#if (SAMD || SAMR21)
#ifndef AT86RFX_SPI
#define AT86RFX_SPI                  SERCOM0
#define AT86RFX_RST_PIN              PIN_PA23
#define AT86RFX_MISC_PIN             PIN_PA23
#define AT86RFX_IRQ_PIN              PIN_PA22
#define AT86RFX_SLP_PIN              PIN_PA24
#define AT86RFX_SPI_CS               PIN_PA19
#define AT86RFX_SPI_MOSI             PIN_PA16
#define AT86RFX_SPI_MISO             PIN_PA18
#define AT86RFX_SPI_SCK              PIN_PA17
#define AT86RFX_CSD                          PIN_PA23
#define AT86RFX_CPS                  PIN_PA23
#define LED0 LED0_PIN

#define AT86RFX_SPI_CONFIG(config) \
  config.mux_setting = SPI_SIGNAL_MUX_SETTING_A; \
  config.mode_specific.master.baudrate = AT86RFX_SPI_BAUDRATE; \
  config.pinmux_pad0 = PINMUX_UNUSED; \
  config.pinmux_pad1 = PINMUX_UNUSED; \
  config.pinmux_pad2 = PINMUX_UNUSED; \
  config.pinmux_pad3 = PINMUX_UNUSED;

#define AT86RFX_IRQ_CHAN             6
#define AT86RFX_INTC_INIT()           struct extint_chan_conf eint_chan_conf; \
  extint_chan_get_config_defaults(&eint_chan_conf); \
  eint_chan_conf.gpio_pin = AT86RFX_IRQ_PIN; \
  eint_chan_conf.gpio_pin_mux = PINMUX_PA22A_EIC_EXTINT6;	\
  eint_chan_conf.gpio_pin_pull      = EXTINT_PULL_NONE; \
  eint_chan_conf.wake_if_sleeping    = true; \
  eint_chan_conf.filter_input_signal = false; \
  eint_chan_conf.detection_criteria  = EXTINT_DETECT_RISING; \
  extint_chan_set_config(AT86RFX_IRQ_CHAN, &eint_chan_conf); \
  extint_register_callback(AT86RFX_ISR, AT86RFX_IRQ_CHAN,	\
    EXTINT_CALLBACK_TYPE_DETECT);

/** Enables the transceiver main interrupt. */
#define ENABLE_TRX_IRQ()                extint_chan_enable_callback( \
    AT86RFX_IRQ_CHAN, EXTINT_CALLBACK_TYPE_DETECT)

/** Disables the transceiver main interrupt. */
#define DISABLE_TRX_IRQ()               extint_chan_disable_callback( \
    AT86RFX_IRQ_CHAN, EXTINT_CALLBACK_TYPE_DETECT)

/** Clears the transceiver main interrupt. */
#define CLEAR_TRX_IRQ()                 extint_chan_clear_detected( \
    AT86RFX_IRQ_CHAN);

/*
 * This macro saves the trx interrupt status and disables the trx interrupt.
 */
#define ENTER_TRX_REGION()   { extint_chan_disable_callback(AT86RFX_IRQ_CHAN, \
                 EXTINT_CALLBACK_TYPE_DETECT)

/*
 *  This macro restores the transceiver interrupt status
 */
#define LEAVE_TRX_REGION()   extint_chan_enable_callback(AT86RFX_IRQ_CHAN, \
    EXTINT_CALLBACK_TYPE_DETECT); \
  }

#endif
#endif /* SAMD || SAMR21 */
#endif /* CONF_TRX_ACCESS_H_INCLUDED */
