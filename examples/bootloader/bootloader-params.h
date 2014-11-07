#ifndef BOOTLOADER_PARAMS_H
#define BOOTLOADER_PARAMS_H

void bootloader_params_set_new_fw_downloaded(uint8_t installed);
uint8_t bootloader_params_get_new_fw_downloaded(void);

void bootloader_params_set_new_fw_boot_ok(uint8_t booted);
uint8_t bootloader_params_get_new_fw_boot_ok(void);

unsigned long fw_version(void);
unsigned long bootloader_version(void);

void bootloader_params_load(void);

#endif // BOOTLOADER_PARAMS_H
