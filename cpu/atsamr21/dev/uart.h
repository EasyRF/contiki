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
#ifndef UART_H
#define UART_H

#include "contiki.h"


typedef int (* uart_rx_char_callback)(uint8_t c);
typedef int (* uart_rx_frame_callback)(void);

struct uart_driver {

  /** \brief Enables the UART controller, configures I/O control and interrupts */
  int (* open)(int32_t baudrate, uart_rx_char_callback char_cb, uart_rx_frame_callback frame_cb);

  /** \brief Disables the UART */
  int (* close)(void);

  /** \brief Sends a single character down the UART
   * \param b The character to transmit
   */
  int (* write_byte)(const unsigned char b);

  /** \brief Sends multiple characters down the UART
   * \param b The characters to transmit
   */
  int (* write_buffer)(const unsigned char  * buffer, uint8_t len);

  /** \brief Set the receive buffer for DMA transfer
   * \param input buffer
   * \param input len
   */
  void (* uart_set_receive_buffer)(unsigned char * buffer, uint8_t len);

};

#endif // UART_H
