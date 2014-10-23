#include "contiki.h"
#include "crc16.h"
#include "log.h"
#include "bootloader-conf.h"
#include "bootloader-params.h"


struct bootloader_params {
  uint32_t bootloader_version;
  uint8_t  new_fw_downloaded;
  uint8_t  new_fw_boot_ok;
  uint16_t crc;
};

static struct bootloader_params config;


/*---------------------------------------------------------------------------*/
static void
save(void)
{
  int status;
  config.crc = crc16_data((unsigned char *)&config, sizeof(config) - 2, 0);
  BOOTLOADER_PARAMS_FLASH.open();
  status = BOOTLOADER_PARAMS_FLASH.write(BOOTLOADER_PARAMS_START_ADDRESS, (const unsigned char *)&config, sizeof(config));
  BOOTLOADER_PARAMS_FLASH.close();

  if (status == -1) {
    FATAL("error saving bootloader config");
  }

  INFO("config saved");
}
/*---------------------------------------------------------------------------*/
void
bootloader_params_set_new_fw_downloaded(uint8_t downloaded)
{
  if (config.new_fw_downloaded != downloaded) {
    config.new_fw_downloaded = downloaded;
    TRACE("new_fw_downloaded = %d", downloaded);
    save();
  }
}
/*---------------------------------------------------------------------------*/
uint8_t
bootloader_params_get_new_fw_downloaded(void)
{
  return config.new_fw_downloaded;
}
/*---------------------------------------------------------------------------*/
void
bootloader_params_set_new_fw_boot_ok(uint8_t ok)
{
  if (config.new_fw_boot_ok != ok) {
    config.new_fw_boot_ok = ok;
    TRACE("new_fw_boot_ok = %d", ok);
    save();
  }
}
/*---------------------------------------------------------------------------*/
uint8_t
bootloader_params_get_new_fw_boot_ok(void)
{
  return config.new_fw_boot_ok;
}
/*---------------------------------------------------------------------------*/
unsigned long
fw_version(void)
{
  return *(unsigned long *)(APPLICATION_FW_VERSION);
}
/*---------------------------------------------------------------------------*/
unsigned long
bootloader_version(void)
{
  return *(unsigned long *)(BOOTLOADER_FW_VERSION);
}
/*---------------------------------------------------------------------------*/
static void
init_defaults(void)
{
  config.bootloader_version = bootloader_version();
  config.new_fw_downloaded  = 0;
  config.new_fw_boot_ok     = 1;
  config.crc                = 0;

  INFO("defaults loaded");
}
/*---------------------------------------------------------------------------*/
void
bootloader_params_load(void)
{
  BOOTLOADER_PARAMS_FLASH.open();
  BOOTLOADER_PARAMS_FLASH.read(BOOTLOADER_PARAMS_START_ADDRESS, (unsigned char *)&config, sizeof(config));
  BOOTLOADER_PARAMS_FLASH.close();
  uint16_t crc = crc16_data((unsigned char *)&config, sizeof(config) - 2, 0);
  if (config.bootloader_version != bootloader_version()) {
    INFO("version not set, first boot of bootloader");
    init_defaults();
    save();
  }
  else if (config.crc != crc) {
    WARN("CRC error in config");
    init_defaults();
    save();
  } else {
    INFO("config loaded");
  }
}
