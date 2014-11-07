#ifndef AT25DF32S_H
#define AT25DF32S_H

#include "flash.h"


#define SST25VF032B_SIZE      (4096UL * 1024UL)


extern const struct flash_driver flash_sst25vf032b;

void sst25vf032b_arch_spi_init(void);
uint8_t sst25vf032b_arch_spi_write(uint8_t data);
uint8_t sst25vf032b_arch_spi_read(void);
void sst25vf032b_arch_spi_select(void);
void sst25vf032b_arch_spi_deselect(void);

#endif // AT25DF32S_H
