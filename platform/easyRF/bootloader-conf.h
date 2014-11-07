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
#ifndef BOOTLOADERCONFIG_H
#define BOOTLOADERCONFIG_H

#include "dev/flash.h"

extern uint32_t _sfixed;
extern uint32_t _header;

#define FIRMWARE_MEM_HEADER_OFFSET          ((unsigned long)(&_header) - (unsigned long)(&_sfixed))

#define FIRMWARE_MEM_HEADER_SIGNATURE       FIRMWARE_MEM_HEADER_OFFSET + 0
#define FIRMWARE_MEM_HEADER_VERSION         FIRMWARE_MEM_HEADER_OFFSET + 8
#define FIRMWARE_MEM_HEADER_LOAD_ADDRESS    FIRMWARE_MEM_HEADER_OFFSET + 12
#define FIRMWARE_MEM_HEADER_START_ADDRESS   FIRMWARE_MEM_HEADER_OFFSET + 16
#define FIRMWARE_MEM_HEADER_END_ADDRESS     FIRMWARE_MEM_HEADER_OFFSET + 20

#define APPLICATION_FW_START_ADDRESS        0x00010000
#define APPLICATION_FW_MAX_SIZE             0x00030000
#define APPLICATION_FW_VERSION              APPLICATION_FW_START_ADDRESS + FIRMWARE_MEM_HEADER_VERSION

#define BOOTLOADER_PARAMS_FLASH             INTERNAL_FLASH
#define BOOTLOADER_FW_START_ADDRESS         0x00000000
#define BOOTLOADER_FW_VERSION               BOOTLOADER_FW_START_ADDRESS + FIRMWARE_MEM_HEADER_VERSION
#define BOOTLOADER_PARAMS_START_ADDRESS     0x00008000

#define BACKUP_FW_START_ADDRESS             0
#define DOWNLOAD_FW_START_ADDRESS           0

#endif // BOOTLOADERCONFIG_H
