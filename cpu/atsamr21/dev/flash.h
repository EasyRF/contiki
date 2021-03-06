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
#ifndef __FLASH_H__
#define __FLASH_H__

#include <stdint.h>


struct flash_driver {

  int (* open)(void);

  void (* close)(void);

  int (* erase)(unsigned long from, unsigned long to);

  int (* read)(unsigned long addr, unsigned char * buffer, unsigned long len);

  int (* write)(unsigned long addr, const unsigned char * buffer, unsigned long len);

  int (* sector_size)(void);

  int (* sector_count)(void);

  int (* page_size)(void);

};

unsigned short flash_crc16(const struct flash_driver * flash, unsigned long address, uint32_t len);

#include "flash-conf.h"

#endif /* __FLASH_H__ */
