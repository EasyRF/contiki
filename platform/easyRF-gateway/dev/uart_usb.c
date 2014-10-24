#include <asf.h>
#include "dev/leds.h"
#include "uart_usb.h"


/*---------------------------------------------------------------------------*/
static int
open(int32_t baudrate, uart_rx_char_callback char_cb, uart_rx_frame_callback frame_cb)
{
  udc_start();

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
close(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
write_byte(const unsigned char b)
{
  if (udi_cdc_putc(b) == 1) {
    return 1;
  } else {
    return -1;
  }
}
/*---------------------------------------------------------------------------*/
static int
write_buffer(const unsigned char * buffer, uint8_t len)
{
  if (udi_cdc_write_buf(buffer, len) == 0) {
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
const struct uart_driver uart_usb =
{
  open,
  close,
  write_byte,
  write_buffer,
  set_receive_buffer
};
