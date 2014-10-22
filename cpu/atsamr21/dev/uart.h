#ifndef UART_H
#define UART_H

#include "contiki.h"


typedef int (* uart_rx_char_callback)(uint8_t c);
typedef int (* uart_rx_frame_callback)(void);

struct uart_driver {

  /** \brief Initialises the UART controller, configures I/O control and interrupts */
  int (* init)(int32_t baudrate, uart_rx_char_callback char_cb, uart_rx_frame_callback frame_cb);

  /** \brief Sends a single character down the UART
   * \param b The character to transmit
   */
  int (* write_byte)(const unsigned char b);

  /** \brief Sends multiple characters down the UART
   * \param b The characters to transmit
   */
  int (* write_buffer)(const unsigned char  * buffer, uint8_t len);

  /** \brief Toggles the usage of TXE for RS485
   * \param input A integer value, non-zero to enable
   */
  void (* uart_use_txe)(int use);

  /** \brief Set the receive buffer for DMA transfer
   * \param input buffer
   * \param input len
   */
  void (* uart_set_receive_buffer)(unsigned char * buffer, uint8_t len);

};

#endif // UART_H
