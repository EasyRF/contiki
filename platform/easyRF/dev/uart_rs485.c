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
#include "uart_rs232.h"


static struct usart_module usart_instance;
static uart_rx_char_callback char_callback;


uint16_t rx_char;

static void
usart_read_callback(struct usart_module *const usart_module)
{
  ENERGEST_ON(ENERGEST_TYPE_IRQ);

  if (char_callback) {
    char_callback(rx_char & 0xff);
  }
  usart_read_job(&usart_instance, &rx_char);

  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}
/*---------------------------------------------------------------------------*/
static int
open(int32_t baudrate, uart_rx_char_callback char_cb, uart_rx_frame_callback frame_cb)
{
  struct usart_config config_usart;
  struct port_config pin_conf;

  usart_get_config_defaults(&config_usart);

  config_usart.baudrate    = baudrate;
  config_usart.mux_setting = RS485_SERCOM_MUX_SETTING;
  config_usart.pinmux_pad0 = RS485_SERCOM_PINMUX_PAD0;
  config_usart.pinmux_pad1 = RS485_SERCOM_PINMUX_PAD1;
  config_usart.pinmux_pad2 = RS485_SERCOM_PINMUX_PAD2;
  config_usart.pinmux_pad3 = RS485_SERCOM_PINMUX_PAD3;

  while (usart_init(&usart_instance, RS485_MODULE, &config_usart) != STATUS_OK) {}

  usart_enable(&usart_instance);

  port_get_config_defaults(&pin_conf);
  pin_conf.direction = PORT_PIN_DIR_OUTPUT;
  port_pin_set_config(RS485_TXE, &pin_conf);
  port_pin_set_output_level(RS485_TXE, false);

  char_callback = char_cb;

  usart_register_callback(&usart_instance,
                          usart_read_callback, USART_CALLBACK_BUFFER_RECEIVED);

  usart_enable_callback(&usart_instance, USART_CALLBACK_BUFFER_RECEIVED);

  usart_read_job(&usart_instance, &rx_char);

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
close(void)
{
  usart_disable(&usart_instance);
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
write_byte(const unsigned char b)
{
  port_pin_set_output_level(RS485_TXE, true);
  if (usart_write_wait(&usart_instance, b) == STATUS_OK) {
    port_pin_set_output_level(RS485_TXE, false);
    return 1;
  } else {
    return -1;
  }
}
/*---------------------------------------------------------------------------*/
static int
write_buffer(const unsigned char * buffer, uint8_t len)
{
  port_pin_set_output_level(RS485_TXE, true);
  if (usart_write_buffer_wait(&usart_instance, buffer, len) == STATUS_OK) {
    port_pin_set_output_level(RS485_TXE, false);
    return len;
  } else {
    return -1;
  }
}
/*---------------------------------------------------------------------------*/
static void
set_receive_buffer(unsigned char * buffer, uint8_t len)
{
  /* TODO */
}
/*---------------------------------------------------------------------------*/
const struct uart_driver uart_rs485 =
{
  open,
  close,
  write_byte,
  write_buffer,
  set_receive_buffer
};
