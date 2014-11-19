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

#ifndef CANVAS_TEXT_H_
#define CANVAS_TEXT_H_

#include "canvas.h"

struct canvas_textbox {
  struct canvas_rectangle rect;
  display_pos_t cursor_x;
  display_pos_t cursor_y;
  display_color_t text_color;
  display_color_t background_color;
  display_color_t border_color;
};

int canvas_text_load_font(const char * filename);

display_pos_t canvas_font_height(int font_fd);

int canvas_text_draw_string(const struct display_driver * display,
                            struct canvas_textbox * textbox,
                            const int font_fd,
                            const char * s);

#endif /* CANVAS_TEXT_H_ */
