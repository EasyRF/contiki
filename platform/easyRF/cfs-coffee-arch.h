#ifndef CFSCOFFEEARCH_H
#define CFSCOFFEEARCH_H

#include "flash-conf.h"


/* Implementation for easyRF external flash memory */
#define COFFEE_SECTOR_SIZE      4096UL
#define COFFEE_PAGE_SIZE        256UL /* Smallest read/write block. UL is needed because: sizeof(COFFEE_PAGE_SIZE) >= sizeof(cfs_offset_t) */
#define COFFEE_START            0UL    /* Start address of memory */
#define COFFEE_SIZE             (1024UL * 1024UL) /* Use 1MB for now */
#define COFFEE_NAME_LENGTH      16
#define COFFEE_MAX_OPEN_FILES   6
#define COFFEE_FD_SET_SIZE      8
#ifdef COFFEE_CONF_DYN_SIZE
#define COFFEE_DYN_SIZE         COFFEE_CONF_DYN_SIZE
#else
#define COFFEE_DYN_SIZE         (1024)
#endif

#define COFFEE_MICRO_LOGS       1
#define COFFEE_LOG_TABLE_LIMIT  256
#define COFFEE_LOG_SIZE         1024
#define COFFEE_IO_SEMANTICS     1
#define COFFEE_APPEND_ONLY      0


#define COFFEE_WRITE(buf, size, offset) \
        flash_driver_coffee_write(COFFEE_START + offset, (const unsigned char *)buf, size)

#define COFFEE_READ(buf, size, offset)  \
        flash_driver_coffee_read(COFFEE_START + offset, (unsigned char *)buf, size)

#define COFFEE_ERASE(sector)  \
        EXTERNAL_FLASH.erase(COFFEE_START + (sector) * COFFEE_SECTOR_SIZE, COFFEE_START + (sector) * COFFEE_SECTOR_SIZE + COFFEE_SECTOR_SIZE)


typedef int16_t coffee_page_t;

void flash_driver_coffee_write(unsigned long addr, const unsigned char * buffer, unsigned long len);
void flash_driver_coffee_read(unsigned long addr, unsigned char * buffer, unsigned long len);

#endif // CFSCOFFEEARCH_H
