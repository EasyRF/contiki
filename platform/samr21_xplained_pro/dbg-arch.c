/*
 * Copyright (c) 2012, Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/** \addtogroup cc2538-char-io
 * @{ */
/**
 * \file
 *     Implementation of arch-specific functions required by the dbg_io API in
 *     cpu/arm/common/dbg-io
 */
#include <stdio.h>
#include <asf.h>

#include "contiki.h"
#include "dbg-arch.h"


/*---------------------------------------------------------------------------*/
#ifndef DBG_CONF_USB
#define DBG_CONF_USB 0
#endif
/*---------------------------------------------------------------------------*/
#undef puts
/*---------------------------------------------------------------------------*/
#if DBG_CONF_USB == 0
struct usart_module usart_instance;
#endif
/*---------------------------------------------------------------------------*/
unsigned int
dbg_send_bytes(const unsigned char *s, unsigned int len)
{
#if DBG_CONF_USB == 0
  if (usart_write_buffer_wait(&usart_instance, (uint8_t *)s, len) == STATUS_OK) {
    return len;
  } else {
    return 0;
  }
#else
  udi_cdc_write_buf(s, len);
  return len;
#endif
}
/*---------------------------------------------------------------------------*/
void
dbg_init()
{
#if DBG_CONF_USB == 0
  struct usart_config config_usart;

  usart_get_config_defaults(&config_usart);
  config_usart.baudrate    = 115200;
  config_usart.mux_setting = EDBG_CDC_SERCOM_MUX_SETTING;
  config_usart.pinmux_pad0 = EDBG_CDC_SERCOM_PINMUX_PAD0;
  config_usart.pinmux_pad1 = EDBG_CDC_SERCOM_PINMUX_PAD1;
  config_usart.pinmux_pad2 = EDBG_CDC_SERCOM_PINMUX_PAD2;
  config_usart.pinmux_pad3 = EDBG_CDC_SERCOM_PINMUX_PAD3;
  while (usart_init(&usart_instance,
          EDBG_CDC_MODULE, &config_usart) != STATUS_OK) {
  }
  usart_enable(&usart_instance);
#else
  udc_start();
#endif
}
/*---------------------------------------------------------------------------*/
void
dbg_uninit(void)
{
//  usart_disable(&usart_instance);
}
/*---------------------------------------------------------------------------*/
int
puts(const char *s)
{
  unsigned int l = strlen(s);
  dbg_send_bytes((const unsigned char *)s, l);
  return l;
}

/** @} */
