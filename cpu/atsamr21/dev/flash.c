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

    left -= 4;
  }

  leds_off((LEDS_GREEN));

  flash->close();

  return acc;
}
