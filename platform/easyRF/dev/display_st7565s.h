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

#ifndef DISPLAY_ST7565S_H_
#define DISPLAY_ST7565S_H_

#include <stdbool.h>
#include <stdint.h>

/* Monochrome display */
typedef uint8_t display_color_t;

/* Resolution fits in a uin8t_t */
typedef uint16_t display_pos_t;

/* The display_driver needs a definition for display_color_t */
#include "display.h"

/* The display */
extern const struct display_driver display_st7565s;

/* ST7565s architecture-specific SPI functions that are called by the
   driver and must be implemented by the platform code */
void st7565s_arch_spi_init(void);
uint8_t st7565s_arch_spi_write(uint8_t data);
void st7565s_arch_spi_select(bool is_data);
void st7565s_arch_spi_deselect(void);
void st7565s_arch_set_backlight(bool on);

#endif /* DISPLAY_ST7565S_H_ */
