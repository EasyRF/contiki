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
#include "uart_rs232.h"
#include "uart_usb.h"


/*---------------------------------------------------------------------------*/
#ifndef DBG_CONF_USB
#define DBG_CONF_USB 0
#endif
/*---------------------------------------------------------------------------*/
#if DBG_CONF_USB == 0
const struct uart_driver * uart = &uart_rs232;
#else
const struct uart_driver * uart = &uart_usb;
#endif
/*---------------------------------------------------------------------------*/
unsigned int
dbg_send_bytes(const unsigned char *s, unsigned int len)
{
  return uart->write_buffer(s, len);
}
/*---------------------------------------------------------------------------*/
void
dbg_init()
{
  /* No receive callback at this point */
  uart->open(115200, NULL, NULL);
}
/*---------------------------------------------------------------------------*/
void
dbg_uninit(void)
{
  uart->close();
}
/*---------------------------------------------------------------------------*/
#undef puts
int
puts(const char *s)
{
  unsigned int l = strlen(s);
  dbg_send_bytes((const unsigned char *)s, l);
  return l;
}

/** @} */
