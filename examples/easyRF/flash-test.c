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
#include "flash.h"
#include "random.h"
#include "log.h"


#define TEST_EXT_FLASH

#ifdef TEST_EXT_FLASH
#define FLASH_DRIVER      EXTERNAL_FLASH
#define ADDRESS_OFFSET    (0)
#else
#define FLASH_DRIVER      INTERNAL_FLASH
#define ADDRESS_OFFSET    (1024 * 128) /* Make sure no program code is overwritten */
#endif


/*---------------------------------------------------------------------------*/
PROCESS(flash_test_process, "Flash-test process");
AUTOSTART_PROCESSES(&flash_test_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(flash_test_process, ev, data)
{
  static struct etimer et;
  int page_size, sector_size;

  PROCESS_BEGIN();

  etimer_set(&et, CLOCK_SECOND * 5);
  PROCESS_WAIT_UNTIL(etimer_expired(&et));

  INFO("Starting FLASH tests");

  /* Open the flash */
  FLASH_DRIVER.open();

  sector_size = FLASH_DRIVER.sector_size();
  page_size = FLASH_DRIVER.page_size();

  if (page_size == -1) {
    INFO("Flash has no pages. Defaulting to 256 for tests");
    page_size = 256;
  }


  {
    unsigned long addr = ADDRESS_OFFSET;
    uint8_t data_in = 0xAB;
    uint8_t data_out = 0;

    INFO("Write 1 byte at start of first page");

    FLASH_DRIVER.erase(addr, addr + sector_size);
    FLASH_DRIVER.write(addr, (const unsigned char *)&data_in, sizeof(data_in));
    FLASH_DRIVER.read(addr, (unsigned char *)&data_out, sizeof(data_out));

    if (data_in == data_out) {
      INFO("OK");
    } else {
      WARN("FAILED");
    }
  }

  {
    unsigned long addr = ADDRESS_OFFSET + page_size - 1;
    uint8_t data_in = 0xAB;
    uint8_t data_out = 0;

    INFO("Write 1 byte at end of first page");

    FLASH_DRIVER.erase(addr, addr + sector_size);
    FLASH_DRIVER.write(addr, (const unsigned char *)&data_in, sizeof(data_in));
    FLASH_DRIVER.read(addr, (unsigned char *)&data_out, sizeof(data_out));

    if (data_in == data_out) {
      INFO("OK");
    } else {
      WARN("FAILED");
    }
  }

  {
    unsigned long addr = ADDRESS_OFFSET;
    unsigned short data_in = 0xABCD;
    unsigned short data_out = 0;

    INFO("Write 2 bytes at start of first page");

    FLASH_DRIVER.erase(addr, addr + sector_size);
    FLASH_DRIVER.write(addr, (const unsigned char *)&data_in, sizeof(data_in));
    FLASH_DRIVER.read(addr, (unsigned char *)&data_out, sizeof(data_out));

    if (data_in == data_out) {
      INFO("OK");
    } else {
      WARN("FAILED");
    }
  }

  {
    unsigned long addr = ADDRESS_OFFSET + page_size - 2;
    unsigned short data_in = 0xABCD;
    unsigned short data_out = 0;

    INFO("Write 2 bytes at end of first page");

    FLASH_DRIVER.erase(addr, addr + sector_size);
    FLASH_DRIVER.write(addr, (const unsigned char *)&data_in, sizeof(data_in));
    FLASH_DRIVER.read(addr, (unsigned char *)&data_out, sizeof(data_out));

    if (data_in == data_out) {
      INFO("OK");
    } else {
      WARN("FAILED");
    }
  }

  {
    unsigned long addr = ADDRESS_OFFSET;
    uint8_t data_in[3];
    uint8_t data_out[3];
    uint32_t i;

    INFO("Write 3 bytes at start of first page");

    for (i = 0; i < sizeof(data_in); i++) {
      data_in[i] = random_rand();
    }

    FLASH_DRIVER.erase(addr, addr + sector_size);
    FLASH_DRIVER.write(addr, (const unsigned char *)&data_in, sizeof(data_in));
    FLASH_DRIVER.read(addr, (unsigned char *)&data_out, sizeof(data_out));

    for (i = 0; i < sizeof(data_in); i++) {
      if (data_in[i] != data_out[i]) {
        break;
      }
    }

    if (i == sizeof(data_in)) {
      INFO("OK");
    } else {
      WARN("FAILED");
    }
  }

  {
    unsigned long addr = ADDRESS_OFFSET + 1;
    uint8_t data_in[3];
    uint8_t data_out[3];
    uint32_t i;

    INFO("Write 3 bytes starting at address 1");

    for (i = 0; i < sizeof(data_in); i++) {
      data_in[i] = random_rand();
    }

    FLASH_DRIVER.erase(addr, addr + sector_size);
    FLASH_DRIVER.write(addr, (const unsigned char *)&data_in, sizeof(data_in));
    FLASH_DRIVER.read(addr, (unsigned char *)&data_out, sizeof(data_out));

    for (i = 0; i < sizeof(data_in); i++) {
      if (data_in[i] != data_out[i]) {
        break;
      }
    }

    if (i == sizeof(data_in)) {
      INFO("OK");
    } else {
      WARN("FAILED");
    }
  }

  {
    unsigned long addr = ADDRESS_OFFSET + page_size - 1;
    unsigned short data_in = 0xABCD;
    unsigned short data_out = 0;

    INFO("Write 2 bytes at boundary of first and second page");

    FLASH_DRIVER.erase(addr, addr + sector_size * 2);
    FLASH_DRIVER.write(addr, (const unsigned char *)&data_in, sizeof(data_in));
    FLASH_DRIVER.read(addr, (unsigned char *)&data_out, sizeof(data_out));

    if (data_in == data_out) {
      INFO("OK");
    } else {
      WARN("FAILED");
    }
  }

  {
    unsigned long addr = ADDRESS_OFFSET + page_size - 10;
    uint8_t data_in[512];
    uint8_t data_out[512];
    uint32_t i;

    INFO("Write 512 bytes starting just before the end of the first page");

    for (i = 0; i < sizeof(data_in); i++) {
      data_in[i] = random_rand();
    }

    FLASH_DRIVER.erase(addr, addr + sector_size * 3);
    FLASH_DRIVER.write(addr, (const unsigned char *)&data_in, sizeof(data_in));
    FLASH_DRIVER.read(addr, (unsigned char *)&data_out, sizeof(data_out));

    for (i = 0; i < sizeof(data_in); i++) {
      if (data_in[i] != data_out[i]) {
        break;
      }
    }

    if (i == sizeof(data_in)) {
      INFO("OK");
    } else {
      WARN("FAILED");
    }
  }


  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
