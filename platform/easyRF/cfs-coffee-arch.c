#include "cfs-coffee-arch.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/
void
flash_driver_coffee_write(unsigned long addr, const unsigned char * buffer, unsigned long len)
{
  unsigned long i;
  unsigned char * ptr = (unsigned char *)buffer;

  for (i = 0; i < len; i++) {
    *ptr = ~(*ptr);
    ptr++;
  }

  EXTERNAL_FLASH.write(addr, buffer, len);
}
/*---------------------------------------------------------------------------*/
void
flash_driver_coffee_read(unsigned long addr, unsigned char * buffer, unsigned long len)
{
  unsigned long i;
  unsigned char * ptr = buffer;

  EXTERNAL_FLASH.read(addr, buffer, len);

  for (i = 0; i < len; i++) {
    *ptr = ~(*ptr);
    ptr++;
  }
}
/*---------------------------------------------------------------------------*/
