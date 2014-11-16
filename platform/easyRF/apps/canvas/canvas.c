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

#include <stdio.h>
#include "compiler.h"
#include "autofs.h"
#include "canvas.h"
#include "log.h"

/*---------------------------------------------------------------------------*/
void
canvas_vline(const struct display_driver * display,
             display_pos_t startx, display_pos_t starty,
             display_pos_t len, uint8_t thickness,
             display_color_t color)
{
  display_pos_t x, y;

  for (y = 0; y < len; y++) {
    for (x = 0; x < thickness; x++) {
      display->set_px(startx + x, starty + y, color);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
canvas_hline(const struct display_driver * display,
             display_pos_t startx, display_pos_t starty,
             display_pos_t len, uint8_t thickness,
             display_color_t color)
{
  display_pos_t x, y;

  for (x = 0; x < len; x++) {
    for (y = 0; y < thickness; y++) {
      display->set_px(startx + x, starty + y, color);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
canvas_line(const struct display_driver * display,
            display_pos_t x, display_pos_t y,
            display_pos_t x2, display_pos_t y2,
            display_color_t color)
{
  int w = x2 - x;
  int h = y2 - y;

  int dx1 = 0, dy1 = 0, dx2 = 0, dy2 = 0;

  if (w<0) dx1 = -1; else if (w>0) dx1 = 1;
  if (h<0) dy1 = -1; else if (h>0) dy1 = 1;
  if (w<0) dx2 = -1; else if (w>0) dx2 = 1;

  int longest  = abs(w);
  int shortest = abs(h);

  if (!(longest > shortest)) {
    longest  = abs(h);
    shortest = abs(w);

    if (h<0) dy2 = -1;
    else if (h>0) dy2 = 1;

    dx2 = 0;
  }

  int numerator = longest >> 1;
  for (int i=0;i<=longest;i++) {
    display->set_px(x, y, color);
    numerator += shortest;
    if (!(numerator<longest)) {
      numerator -= longest;
      x += dx1;
      y += dy1;
    } else {
      x += dx2;
      y += dy2;
    }
  }
}
/*---------------------------------------------------------------------------*/
void
canvas_fill(const struct display_driver * display,
            display_pos_t startx, display_pos_t starty,
            display_pos_t width, display_pos_t height,
            display_color_t color)
{
  display_pos_t x, y;

  for (x = startx; x < (startx + width); x++) {
    for (y = starty; y < (starty + height); y++) {
      display->set_px(x, y, color);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
canvas_invert(const struct display_driver * display,
              display_pos_t startx, display_pos_t starty,
              display_pos_t width, display_pos_t height)
{
  display_pos_t x, y;

  for (x = startx; x < (startx + width); x++) {
    for (y = starty; y < (starty + height); y++) {
      display->invert_px(x, y);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
canvas_bmp(const struct display_driver * display,
           const char * filename,
           display_pos_t offsetx, display_pos_t offsety,
           display_color_t color)
{
  int fd;
  struct BMP_HEADER hdr;
  int bytes_per_row;
  display_pos_t x, y;
  uint8_t bitmask = 0, image_byte = 0;

  fd = autofs_open(filename, CFS_READ);
  if (fd < 0) {
    WARN("Cannot find %s", filename);
    return;
  }

  if (autofs_read(fd, (void *)&hdr, sizeof(struct BMP_HEADER)) != sizeof(struct BMP_HEADER)) {
    WARN("Failed to read BMP header");
  }

  /* Log BMP info */
  INFO("BMP size %dx%d bits %d", hdr.width, hdr.height, hdr.bits);

  /* Check color depth */
  if (hdr.bits != 1) {
    WARN("Only monochrome bitmaps are currently supported");
    goto end;
  }

  /* Bytes in a row are aligned with 32 bits */
  bytes_per_row = (((uint32_t)hdr.bits * hdr.width + 31) / 32) << 2;

  /* Move pointer in file to start of imagedata */
  autofs_seek(fd, hdr.offset, CFS_SEEK_SET);

  /* handle content */
  for (y = 0; y< hdr.height; y++) {
    for (x = 0; x < hdr.width; x++) {
      if ((x % 8) == 0) {
        bitmask = 0x80;
        autofs_read(fd, &image_byte, 1);
      }
      display->set_px(x+offsetx, hdr.height-(y+offsety)-1, ((image_byte & bitmask) != bitmask) ? color : 0);
      bitmask >>= 1;
    }

    /* Discard padding bytes */
    while ((x >> 3) < bytes_per_row) {
      autofs_seek(fd, 1, CFS_SEEK_CUR);
      x += 8;
    }
  }

end:
  autofs_close(fd);
}
