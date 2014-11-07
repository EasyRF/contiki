#include "contiki.h"
#include "flash.h"
#include "log.h"


//#define FLASH_DRIVER  INTERNAL_FLASH
#define FLASH_DRIVER  EXTERNAL_FLASH


/*---------------------------------------------------------------------------*/
PROCESS(flash_test_process, "Flash-test process");
AUTOSTART_PROCESSES(&flash_test_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(flash_test_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

  etimer_set(&et, CLOCK_SECOND * 5);
  PROCESS_WAIT_UNTIL(etimer_expired(&et));

  INFO("Flash test");

  /* Open the flash */
  FLASH_DRIVER.open();

  {
    unsigned long addr = 64 * (4096 - 1);
    unsigned short data_in = 0xABCD;
    unsigned short data_out = 0;

    INFO("Test1: write 2 bytes at the start of the last page");

    FLASH_DRIVER.write(addr, (const unsigned char *)&data_in, sizeof(data_in));
    FLASH_DRIVER.read(addr, (unsigned char *)&data_out, sizeof(data_out));

    if (data_in == data_out) {
      INFO("OK");
    } else {
      WARN("FAILED");
    }
  }


  {
    unsigned long addr = 64 * (4096 - 1);
    unsigned char data_in[64];
    unsigned char data_out[64];
    int i;

    INFO("Test2: write 64 bytes at the start of the last page");

    for (i = 0; i < sizeof(data_in); i++) {
      data_in[i] = i;
    }

    memset(data_out, 0, sizeof(data_out));

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
    unsigned long addr = (64 * (4096 - 2)) + 32;
    unsigned short data_in = 0xABCD;
    unsigned short data_out = 0;

    INFO("Test3: write 2 bytes in the middle of the second last page");

    FLASH_DRIVER.write(addr, (const unsigned char *)&data_in, sizeof(data_in));
    FLASH_DRIVER.read(addr, (unsigned char *)&data_out, sizeof(data_out));

    if (data_in == data_out) {
      INFO("OK");
    } else {
      WARN("FAILED");
    }
  }


  {
    unsigned long addr = (64 * (4096 - 2)) + 32;
    unsigned char data_in[64];
    unsigned char data_out[64];
    int i;

    INFO("Test4: write 64 bytes starting in the middle of the second last page");

    for (i = 0; i < sizeof(data_in); i++) {
      data_in[i] = i;
    }

    memset(data_out, 0, sizeof(data_out));

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
    unsigned long addr = (64 * (4096 - 3)) + 32;
    unsigned char data_in[128];
    unsigned char data_out[128];
    int i;

    INFO("Test5: write 128 bytes starting in the middle of third last page");

    for (i = 0; i < sizeof(data_in); i++) {
      data_in[i] = i;
    }

    memset(data_out, 0, sizeof(data_out));

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
    unsigned long addr = (64 * 4096) - 384;
    unsigned char data_in[384];
    unsigned char data_out[384];
    int i;

    INFO("Test6: write 384 (1.5 times erase size) bytes starting in the middle of third last page");

    for (i = 0; i < sizeof(data_in); i++) {
      data_in[i] = i & 0xff;
    }

    memset(data_out, 0, sizeof(data_out));

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
