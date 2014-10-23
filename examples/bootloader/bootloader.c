#include "contiki.h"
#include "dbg-arch.h"
#include "dev/leds.h"
#include "dev/watchdog.h"
#include "dev/flash.h"
#include "log.h"
#include "crc16.h"
#include "bootloader-conf.h"
#include "bootloader-params.h"


enum flash_copy_direction {
  INTERNAL_TO_EXTERNAL,
  EXTERNAL_TO_INTERNAL
};


/* Forward declaration, implemented in boot-arch */
void boot_firmware(unsigned long address);

/*---------------------------------------------------------------------------*/
static void
blink_leds(uint8_t times)
{
  int i;

  for (i = 0; i < times * 2; i++) {
    leds_toggle(LEDS_GREEN);
    clock_wait(10);
    watchdog_periodic();
  }

  leds_off(LEDS_ALL);
}
/*---------------------------------------------------------------------------*/
static int
copy_flash_to_flash(unsigned long from, unsigned long to, unsigned long len, enum flash_copy_direction dir)
{
  uint16_t crc_read, crc_calculated;

  /* Init source and destination */
  EXTERNAL_FLASH.open();
  INTERNAL_FLASH.open();

  if (dir == INTERNAL_TO_EXTERNAL) {
    TRACE("start copying from %08lX to %08lX in direction %s", from, to, dir == INTERNAL_TO_EXTERNAL ? "internal to external" : "external to internal");

    /* We don't have to use the read function for INTERNAL_FLASH since it is memory mapped */
    if (EXTERNAL_FLASH.write(to, (const unsigned char *)from, len) != 1) {
      WARN("copy_flash_to_flash: error writing to external flash at %08lX", to);
      return -1;
    }

    TRACE("calculating CRC from external flash");

    /* Check CRC */
    EXTERNAL_FLASH.read(to + len - 2, (unsigned char *)&crc_read, 2);
    crc_calculated = flash_crc16(&EXTERNAL_FLASH, to, len - 2);
    if (crc_calculated != crc_read) {
      WARN("copy_flash_to_flash: crc error after copying to external flash. crc_calculated = %d, crc_read = %d", crc_calculated, crc_read);
      return -1;
    }

    TRACE("external flash CRC OK");

    /* Done */
    return 1;
  } else if (dir == EXTERNAL_TO_INTERNAL) {
    static unsigned char data[2048];
    unsigned long i, left;
    unsigned long read_len;

    left = len;

    TRACE("start copying from %08lX to %08lX in direction %s", from, to, dir == INTERNAL_TO_EXTERNAL ? "internal to external" : "external to internal");

    /* Copy in chunks of 2048 bytes */
    for (i = 0; i < len; i += sizeof(data)) {
      read_len = left < sizeof(data) ? left : sizeof(data);
      EXTERNAL_FLASH.read(from + i, data, read_len);
      if (INTERNAL_FLASH.write(to + i, data, read_len) != 1) {
        WARN("copy_flash_to_flash: error writing to internal flash at %08lX", to + i);
        return -1;
      }
      left -= sizeof(data);
    }

    TRACE("calculating CRC from internal flash");

    /* Check CRC */
    INTERNAL_FLASH.read(to + len - 2, (unsigned char *)&crc_read, 2);
    crc_calculated = crc16_data((unsigned char *)to, len - 2, 0);
    if (crc_calculated != crc_read) {
      WARN("copy_flash_to_flash: crc error after copying to internal flash. crc_calculated = %d, crc_read = %d", crc_calculated, crc_read);
      return -1;
    }

    TRACE("internal flash CRC OK");

    /* Done */
    return 1;
  }
  else {
    return -1;
  }
}
/*---------------------------------------------------------------------------*/
static int
backup_application(void)
{
  INFO("Make application backup");

  if (copy_flash_to_flash(APPLICATION_FW_START_ADDRESS, BACKUP_FW_START_ADDRESS, APPLICATION_FW_MAX_SIZE, INTERNAL_TO_EXTERNAL) == -1) {
    WARN("backup failed");
    return -1;
  }

  INFO("Application backup made");

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
restore_application(void)
{
  INFO("Restoring application...");

  if (copy_flash_to_flash(BACKUP_FW_START_ADDRESS, APPLICATION_FW_START_ADDRESS, APPLICATION_FW_MAX_SIZE, EXTERNAL_TO_INTERNAL) == -1) {
    WARN("Restore failed");
    return -1;
  }

  INFO("Application restored");

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
install_application(void)
{
  INFO("Installing new application");

  if (copy_flash_to_flash(DOWNLOAD_FW_START_ADDRESS, APPLICATION_FW_START_ADDRESS, APPLICATION_FW_MAX_SIZE, EXTERNAL_TO_INTERNAL) == -1) {
    WARN("Installing failed");
    return -1;
  }

  INFO("New application installed");

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
verify_application_crc(void)
{
  uint16_t crc_read, crc_calculated;

  /* Check if an application is installed */
  if (fw_version() == -1) {
    return -1;
  }

  /* Read CRC */
  INTERNAL_FLASH.open();
  INTERNAL_FLASH.read(APPLICATION_FW_START_ADDRESS + APPLICATION_FW_MAX_SIZE - 2, (unsigned char *)&crc_read, 2);
  INTERNAL_FLASH.close();

  /* Calculate CRC */
  crc_calculated = crc16_data((unsigned char *)APPLICATION_FW_START_ADDRESS, APPLICATION_FW_MAX_SIZE - 2, 0);

  /* Debug */
  TRACE("crc_read: %d, crc_calculated: %d", crc_read, crc_calculated);

  /* Verify CRC */
  if (crc_calculated != crc_read) {
    if (crc_read != 0xFFFF) {
      WARN("crc error (did you clear all memory before programming target?)");
    }
    /* First boot, write calculated CRC */
    INTERNAL_FLASH.open();
    INTERNAL_FLASH.write(APPLICATION_FW_START_ADDRESS + APPLICATION_FW_MAX_SIZE - 2, (const unsigned char *)&crc_calculated, 2);
    INTERNAL_FLASH.close();
    INFO("No application CRC found, wrote calculated CRC");
  }
  /* Done */
  return 1;
}
/*---------------------------------------------------------------------------*/
int
main(void)
{
//  /* Initilize clock */
  clock_init();

//  /* Initialize watchdog */
//  watchdog_init();
//  watchdog_start();

  /* Init leds */
  leds_init();

  /* Blink leds 4 times */
  blink_leds(4);

  /* Initialize uart or serial_usb, depending on setting in dbg-arch.c */
  dbg_init();

  /* Logging */
  INFO("Starting bootloader");

  /* Load bootload config */
  bootloader_params_load();

  /* Log versions */
  INFO("Bootloader version %ld, application version %ld", bootloader_version(), fw_version());

  /* Verify appliction CRC */
  if (verify_application_crc() != 1) {
    FATAL("No application installed");
  }

  /* Install update or restore backup when neccesary */
  if (!bootloader_params_get_new_fw_boot_ok()) {
    INFO("Last application not ok");
    if (restore_application() == 1) {
      bootloader_params_set_new_fw_boot_ok(1);
    }
  } else if (bootloader_params_get_new_fw_downloaded()) {
    INFO("New firmware found");

    if (backup_application() == 1) {
      if (install_application() == 1) {
        bootloader_params_set_new_fw_boot_ok(0);
      }
    }

    bootloader_params_set_new_fw_downloaded(0);
  }

  /* Start application */
  INFO("Starting application");

  /* Last time before starting application */
//  watchdog_periodic();

  /* Need to uninit uart, otherwise booting application fails */
  dbg_uninit();

  /* Jump to application code */
  boot_firmware(APPLICATION_FW_START_ADDRESS);

  /* Should never get here */
  FATAL("End of the world!!");
}
