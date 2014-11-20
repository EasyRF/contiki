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

#include "display.h"

struct canvas_point {
  display_pos_t x;
  display_pos_t y;
};

struct canvas_rectangle {
  display_pos_t left;
  display_pos_t top;
  display_pos_t width;
  display_pos_t height;
};

void canvas_draw_line   (const struct display_driver * display,
                         const struct canvas_point * p1,
                         const struct canvas_point * p2,
                         const display_color_t line_color);

void canvas_draw_rect   (const struct display_driver * display,
                         const struct canvas_rectangle * rect,
                         const display_color_t line_color,
                         const display_color_t fill_color);

void canvas_draw_bmp    (const struct display_driver * display,
                         const char * filename,
                         const struct canvas_point * point,
                         const display_color_t fg_color,
                         const display_color_t bg_color);

#endif /* CANVAS_H_ */
