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
canvas_draw_line(const struct display_driver * display,
                 const struct canvas_point * p1,
                 const struct canvas_point * p2,
                 const display_color_t color)
{
  display_pos_t x, y, x2, y2;

  x = p1->x;
  y = p1->y;
  x2 = p2->x;
  y2 = p2->y;

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
canvas_draw_rect(const struct display_driver * display,
                 const struct canvas_rectangle * rect,
                 const display_color_t line_color,
                 const display_color_t fill_color)
{
  display_pos_t x, y;

  for (x = rect->left; x < (rect->left + rect->width); x++) {
    for (y = rect->top; y < (rect->top + rect->height); y++) {
      if (x == rect->left || x == rect->left + rect->width  - 1 ||
          y == rect->top  || y == rect->top  + rect->height - 1) {
        display->set_px(x, y, line_color);
      } else {
        display->set_px(x, y, fill_color);
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
void
canvas_bmp(const struct display_driver * display,
           const char * filename,
           const struct canvas_point * point,
           const display_color_t fg_color,
           const display_color_t bg_color)
{
  int fd;
  struct bmp_header hdr;
  int bytes_per_row;
  display_pos_t x, y;
  uint8_t bitmask = 0, image_byte = 0;

  fd = autofs_open(filename, CFS_READ);
  if (fd < 0) {
    WARN("Cannot find %s", filename);
    return;
  }

  if (autofs_read(fd, (void *)&hdr, sizeof(struct bmp_header)) != sizeof(struct bmp_header)) {
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
      display->set_px(point->x + x, hdr.height - (point->y + y) - 1, ((image_byte & bitmask) != bitmask) ? fg_color : bg_color);
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
