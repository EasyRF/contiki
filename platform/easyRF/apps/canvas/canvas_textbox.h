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

/* Convenience function for initializing textbox fields */
static inline void canvas_textbox_init(struct canvas_textbox * textbox,
                                       const display_pos_t left,
                                       const display_pos_t width,
                                       const display_pos_t top,
                                       const display_pos_t height,
                                       const display_color_t text_color,
                                       const display_color_t background_color,
                                       const display_color_t border_color)
{
  textbox->rect.left = left;
  textbox->rect.width = width;
  textbox->rect.top = top;
  textbox->rect.height = height;
  textbox->text_color = text_color;
  textbox->background_color = background_color;
  textbox->border_color = border_color;
  textbox->cursor_x = 0;
  textbox->cursor_y = 0;
}

/* Load the font with the given filename */
int canvas_load_font(const char * filename);

/* Unload the font */
void canvas_unload_font(int font_fd);

/* Get the maximum character height of the font */
display_pos_t canvas_font_height(int font_fd);

/* Clear the contents of the textbox */
void canvas_textbox_clear(const struct display_driver * display,
                          struct canvas_textbox * textbox);

/* Draw a string in the textbox, without cursor reset.
 * It will behave as a string append funtion */
int canvas_textbox_draw_string(const struct display_driver * display,
                               struct canvas_textbox * textbox,
                               const int font_fd,
                               const char * s);

/* Draw a string in the textbox, with a cursor reset
 * It will overwrite the existing text */
int canvas_textbox_draw_string_reset(const struct display_driver * display,
                                     struct canvas_textbox * textbox,
                                     const int font_fd,
                                     const char * s);

#endif /* CANVAS_TEXT_H_ */
