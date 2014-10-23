/*
 *  \file
 *      flash.h
 *   \author
 *      Matthijs Boelstra
 *   \desc
 *      Generic Flash driver interface
 */

#ifndef __FLASH_H__
#define __FLASH_H__

struct flash_driver {

  int (* open)(void);

  void (* close)(void);

  int (* erase)(unsigned long from, unsigned long to);

  int (* read)(unsigned long addr, unsigned char * buffer, unsigned long len);

  int (* write)(unsigned long addr, const unsigned char * buffer, unsigned long len);

};

unsigned short flash_crc16(const struct flash_driver * flash, unsigned long address, uint32_t len);

#include "flash-conf.h"

#endif /* __FLASH_H__ */
