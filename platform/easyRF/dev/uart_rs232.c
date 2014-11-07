#include <asf.h>
#include "uart_rs232.h"


static struct usart_module usart_instance;

/*---------------------------------------------------------------------------*/
static int
open(int32_t baudrate, uart_rx_char_callback char_cb, uart_rx_frame_callback frame_cb)
{
  struct usart_config config_usart;
  usart_get_config_defaults(&config_usart);

  config_usart.baudrate    = baudrate;
  config_usart.mux_setting = RS232_SERCOM_MUX_SETTING;
  config_usart.pinmux_pad0 = RS232_SERCOM_PINMUX_PAD0;
  config_usart.pinmux_pad1 = RS232_SERCOM_PINMUX_PAD1;
  config_usart.pinmux_pad2 = RS232_SERCOM_PINMUX_PAD2;
  config_usart.pinmux_pad3 = RS232_SERCOM_PINMUX_PAD3;

  while (usart_init(&usart_instance, RS232_MODULE, &config_usart) != STATUS_OK) {}

  usart_enable(&usart_instance);

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
  if (usart_write_wait(&usart_instance, b) == STATUS_OK) {
    return 1;
  } else {
    return -1;
  }
}
/*---------------------------------------------------------------------------*/
static int
write_buffer(const unsigned char * buffer, uint8_t len)
{
  if (usart_write_buffer_wait(&usart_instance, buffer, len) == STATUS_OK) {
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
const struct uart_driver uart_rs232 =
{
  open,
  close,
  write_byte,
  write_buffer,
  set_receive_buffer
};
