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

#ifndef CANVAS_H_
#define CANVAS_H_

#include "display_driver.h"

struct BMP_HEADER {
  unsigned short int signature;                /* Signature                   */
  unsigned int file_size;                      /* File size in bytes          */
  unsigned short int reserved1, reserved2;     /* Reserved fields             */
  unsigned int offset;                         /* Offset to image data, bytes */
  unsigned int size;                           /* Header size in bytes        */
  int width,height;                            /* Width and height of image   */
  unsigned short int planes;                   /* Number of colour planes     */
  unsigned short int bits;                     /* Bits per pixel              */
  unsigned int compression;                    /* Compression type            */
  unsigned int imagesize;                      /* Image size in bytes         */
  int xresolution,yresolution;                 /* Pixels per meter            */
  unsigned int ncolours;                       /* Number of colours           */
  unsigned int importantcolours;               /* Important colours           */
} __attribute__ ((packed));


void canvas_vline       (const struct display_driver * display,
                         display_pos_t startx, display_pos_t starty, display_pos_t len,
                         uint8_t thickness, display_color_t color);

void canvas_hline       (const struct display_driver * display,
                         display_pos_t startx, display_pos_t starty, display_pos_t len,
                         uint8_t thickness, display_color_t color);

void canvas_line        (const struct display_driver * display,
                        display_pos_t x0, display_pos_t y0,
                        display_pos_t x1, display_pos_t y1,
                        display_color_t color);

void canvas_fill        (const struct display_driver * display,
                         display_pos_t startx, display_pos_t starty,
                         display_pos_t width, display_pos_t height,
                         display_color_t color);

void canvas_invert      (const struct display_driver * display,
                         display_pos_t startx, display_pos_t starty,
                         display_pos_t width, display_pos_t height);

void canvas_bmp         (const struct display_driver * display,
                         const char * filename,
                         display_pos_t offsetx, display_pos_t offsety,
                         display_color_t color);

#endif /* CANVAS_H_ */
