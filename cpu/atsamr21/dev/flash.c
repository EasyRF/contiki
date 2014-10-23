#include "contiki.h"
#include "dev/watchdog.h"
#include "dev/leds.h"
#include "dev/flash.h"
#include "log.h"
#include "crc16.h"


unsigned short
flash_crc16(const struct flash_driver * flash, unsigned long address, uint32_t len)
{
  unsigned long i, left;
  uint8_t read_len;
  unsigned short acc;
  unsigned char data[4];

  flash->open();

  left = len;
  acc = 0;
  for (i = 0; i < len; i += 4) {
    read_len = left < 4 ? left : 4;
    /* Read read_len bytes */
    flash->read(address + i, data, read_len);
    /* Calculate CRC over read_len bytes */
    acc = crc16_data(data, read_len, acc);
    /* Prevent watchdog from expiring */
    watchdog_periodic();
    if (i % 2048) {
      leds_toggle(LEDS_GREEN);
    }
    left -= 4;
  }

  leds_off((LEDS_GREEN));

  flash->close();

  return acc;
}
