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
#ifndef ASF_H
#define ASF_H

// From module: Common SAM D20 compiler driver
#include <compiler.h>
#include <status_codes.h>

// From module: Generic board support
#include <board.h>

// From module: Interrupt management - SAM implementation
#include <interrupt.h>

// From module: PORT - GPIO Pin Control
#include <port.h>

// From module: Part identification macros
#include <parts.h>

// From module: SERCOM
#include <sercom.h>
#include <sercom_interrupt.h>

// From module: SERCOM USART - Serial Communications (Callback APIs)
#include <usart.h>
#include <usart_interrupt.h>

// From module: SYSTEM - Clock Management for SAMR21
#include <clock.h>
#include <gclk.h>

// From module: SYSTEM - Core System Driver
#include <system.h>

// From module: SYSTEM - I/O Pin Multiplexer
#include <pinmux.h>

// From module: SYSTEM - Interrupt Driver
#include <system_interrupt.h>

// From module: RTC - Real Time Counter in Count Mode (Callback APIs)
#include <rtc_count.h>
#include <rtc_count_interrupt.h>

// From module: Delay routines
#include <delay.h>

// From module: WDT - Watchdog Timer (Polled APIs)
#include <wdt.h>

// From module: EXTINT - External Interrupt (Callback APIs)
#include <extint.h>
#include <extint_callback.h>

// From module: SERCOM SPI - Serial Peripheral Interface (Polled APIs)
#include <spi.h>

// From module: NVM - Non Volatile Memory Interface
#include <nvm.h>

// Fom module: Sleep manager
#include <samd/sleepmgr.h>
#include <sleepmgr.h>

// From module: USB - Universal Serial Bus
#include <usb.h>

// From module: USB CDC Protocol
#include <usb_protocol_cdc.h>

// From module: USB Device CDC (Single Interface Device)
#include <udi_cdc.h>

// From module: USB Device Stack Core (Common API)
#include <udc.h>
#include <udd.h>

// From module: SERCOM I2C - Master Mode I2C (Polled APIs)
#include <i2c_common.h>
#include <i2c_master.h>

// From module: TC
#include <tc.h>
#include <tc_interrupt.h>

// From module TCC
#include <tcc.h>

// From module ADC
#include <adc.h>

// From module: Atmel QTouch Library 5.0 for Atmel SAM4S devices
#include <touch_api_SAMD.h>

#endif // ASF_H
