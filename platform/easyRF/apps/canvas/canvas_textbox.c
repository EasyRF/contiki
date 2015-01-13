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

#include "compiler.h"
#include "autofs.h"
#include "bmp_header.h"
#include "canvas_textbox.h"
#include "log.h"

#undef TRACE
#define TRACE(...)


/* Maximum height of character */
#define FONT_MAX_HEIGHT       32

/* Maximum width of character */
#define FONT_MAX_WIDTH        32

/* First character in bitmap file */
#define FONT_FIRST_CH         0x21

/* Last character in bitmap file */
#define FONT_LAST_CH          0x7E

/* Number of characters in bitmap file */
#define FONT_MAX_CHARS        (FONT_LAST_CH - FONT_FIRST_CH + 1)

/* Maximum number of pixels in a row */
#define FONT_MAX_PX_PER_ROW   (FONT_MAX_WIDTH * FONT_MAX_CHARS)

/* Maximum number of fonts in use */
#define FONT_FD_SET_SIZE      3
#define FONT_FD_FREE          0x0
#define FONT_FD_USED          0x1

/* File descriptor macros. */
#define FD_VALID(fd)					\
  ((fd) >= 0 && (fd) < FONT_FD_SET_SIZE && 	\
  font_fd_set[(fd)].flags != FONT_FD_FREE)


struct font {
  /* File descriptor of current font */
  int font_file_fd;

  /* The offset in the file to the image data */
  cfs_offset_t fontfile_data_offset;

  /* Number of bytes per (horizontal) line in the image */
  cfs_offset_t bytes_per_row;

  /* Height of the font */
  display_pos_t font_height;

  /* The x position of all chars */
  display_pos_t chars_x_position[FONT_MAX_CHARS];

  /* The width of all the chars */
  display_pos_t chars_width[FONT_MAX_CHARS];

  /* FD state flags */
  uint8_t flags;
};


static struct font font_fd_set[FONT_FD_SET_SIZE];

/*---------------------------------------------------------------------------*/

/* Macro's for calculating pixels left for x and y direction within textbox */
#define PX_AVAILX(tb)   ((tb->rect.left + tb->rect.width ) - tb->cursor_x)
#define PX_AVAILY(tb)   ((tb->rect.top  + tb->rect.height) - tb->cursor_y)

/*---------------------------------------------------------------------------*/
static int
get_available_fd(void)
{
  int i;

  for (i = 0; i < FONT_FD_SET_SIZE; i++) {
    if(font_fd_set[i].flags == FONT_FD_FREE) {
      return i;
    }
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
static bool
load_font(struct font * fdp)
{
  struct bmp_header hdr;
  display_pos_t x, y, tmp_x = 0;
  uint8_t flattened_row[FONT_MAX_PX_PER_ROW / 8];
  bool curr_pixel_value = 0, last_pixel_value = 0;
  uint8_t curr_ch = 0, bitmask = 0, currbyte = 0;

  /* Clear position and width arrays */
  memset(fdp->chars_x_position, 0, sizeof(fdp->chars_x_position));
  memset(fdp->chars_width, 0, sizeof(fdp->chars_width));

  /* Clear temp row array */
  memset(flattened_row, 0, sizeof(flattened_row));

  /* Read BMP header */
  if (autofs_read(fdp->font_file_fd, (void *)&hdr,
                  sizeof(struct bmp_header)) != sizeof(struct bmp_header)) {
    WARN("Failed to read BMP header");
    return false;
  }

  /* Check BMP signature */
  if (hdr.signature != BMP_SIGNATURE) {
    WARN("Invalid BMP signature");
    return false;
  }

  /* Log BMP info */
  INFO("BMP size %dx%d bits %d", hdr.width, hdr.height, hdr.bits);

  /* Check character width */
  if (hdr.width > FONT_MAX_PX_PER_ROW) {
    WARN("Unable to load font, BPM to wide!");
    return false;
  }

  /* Check color depth */
  if (hdr.bits != 1) {
    WARN("Only monochrome bitmaps are currently supported");
    return false;
  }

  /* Calculate bytes per row, in BMP's it is always a multiple of 4 bytes */
  fdp->bytes_per_row = (((uint32_t)hdr.bits * hdr.width + 31) / 32) << 2;

  /* Store font height */
  fdp->font_height = hdr.height;

  /* Store offset to image data */
  fdp->fontfile_data_offset = hdr.offset;

  /* Move pointer in file to start of image data */
  autofs_seek(fdp->font_file_fd, fdp->fontfile_data_offset, CFS_SEEK_SET);

  /* OR pixel values of each column */
  for (y = 0; y < hdr.height; y++) {
    for (x = 0; x < fdp->bytes_per_row; x++) {
      /* Read next byte */
      autofs_read(fdp->font_file_fd, &currbyte, 1);
      /* OR pixel for each x position. Black pixels are 0, so invert value */
      flattened_row[x] |= ~currbyte;
    }
  }

  /* Walk through flattened_row to fill charx (startposition of char)
   * and charw (width of char) */
  for (x = 0; x < hdr.width; x++) {

    /* New byte */
    if ((x % 8) == 0) {
      /* Init bitmask */
      bitmask = 0x80;
      /* Retrieve byte for x position */
      currbyte = flattened_row[x >> 3];
    }

    /* Make sure to add a white px to the end of each row
     * to avoid missing the last character */
    curr_pixel_value = (currbyte & bitmask) == bitmask;

    /* Check for (flattened) pixel value change */
    if (last_pixel_value != curr_pixel_value) {
      /* Check for start of a character */
      if (curr_pixel_value) {
        /* Remember the x position */
        tmp_x = x;
      } else {
        /* Check bound of position and width arrays */
        if (curr_ch < FONT_MAX_CHARS) {
          /* This is the end of a character */
          /* Store x position, which is the start of the character */
          fdp->chars_x_position[curr_ch] = tmp_x;
          /* Calculate and store the width of the character */
          fdp->chars_width[curr_ch] = x - tmp_x;
          /* Log character info */
          TRACE("CHAR %c starts @  %d and is %d wide.", curr_ch +
                FONT_FIRST_CH, fdp->chars_x_position[curr_ch], fdp->chars_width[curr_ch]);
          /* Increase array index */
          curr_ch++;
        } else {
          WARN("Not enough room for new character");
        }
      }
      /* Save pixel value */
      last_pixel_value = curr_pixel_value;
    }
    /* Next bit */
    bitmask >>= 1;
  }

  /* Log number of character processed */
  INFO("Read %d chars (max is %d)", curr_ch, FONT_MAX_CHARS);

  /* Ok */
  return true;
}
/*---------------------------------------------------------------------------*/
int
canvas_load_font(const char * filename)
{
  struct font * fdp;
  int fd;

  fd = get_available_fd();
  if(fd < 0) {
    WARN("Failed to allocate a new font descriptor!");
    return -1;
  }

  fdp = &font_fd_set[fd];

  /* Try opening font file */
  fdp->font_file_fd = autofs_open(filename, CFS_READ);
  if (fdp->font_file_fd < 0) {
    WARN("Could not find %s", filename);
    return -1;
  }

  if (!load_font(fdp)) {
    WARN("Error analyzing font %s", filename);
    autofs_close(fdp->font_file_fd);
    return -1;
  }

  fdp->flags = FONT_FD_USED;

  return fd;
}
/*---------------------------------------------------------------------------*/
void
canvas_unload_font(int font_fd)
{
  struct font * fdp;

  if (FD_VALID(font_fd)) {
    fdp = &font_fd_set[font_fd];

    autofs_close(fdp->font_file_fd);

    font_fd_set[font_fd].flags = FONT_FD_FREE;
  }
}
/*---------------------------------------------------------------------------*/
static void
canvas_putch_to_position(const struct display_driver * display,
                         const struct font * fdp,
                         struct canvas_textbox * textbox,
                         const char c)
{
  display_pos_t char_file_offset, x, y;
  uint8_t currbyte, bitmask;
  uint8_t char_width;

  /* Get width of current character */
  char_width = fdp->chars_width[c - FONT_FIRST_CH];

  /* Loop over vertical pixels of character */
  for (y = 0; y < fdp->font_height; y++) {

    /* Get start of file offset of current character */
    char_file_offset = fdp->chars_x_position[c - FONT_FIRST_CH];

    /* Calculate position in file
     * (BMP is always upsidedown, so reversed order for y coordinate) */
    int offset = fdp->fontfile_data_offset +
        (long)fdp->bytes_per_row *
        (fdp->font_height - 1 - y) + (char_file_offset >> 3);

    /* Set file pointer to calculated offset */
    autofs_seek(fdp->font_file_fd, offset, CFS_SEEK_SET);

    /* Read one byte */
    autofs_read(fdp->font_file_fd, &currbyte, 1);

    /* Init bitmask */
    bitmask = 0x80 >> (char_file_offset % 8);

    /* Loop over horizontal pixels of character */
    for(x = 0; x < char_width; x++) {

      /* Check bit state, if set apply text_color for pixel */
      if ((currbyte & bitmask) != bitmask) {
        display->set_px(textbox->cursor_x + x, textbox->cursor_y + y,
                        textbox->text_color);
      }

      /* Next pixel */
      bitmask >>= 1;

      /* Check for all bits done */
      if (bitmask == 0) {

        /* Read next byte from file */
        autofs_read(fdp->font_file_fd, &currbyte, 1);

        /* Reset bitmask */
        bitmask = 0x80;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static bool
draw_crlf(const struct font * fdp,
          struct canvas_textbox * textbox)
{
  TRACE("font heigth: %d, PX_AVAILY: %d", fdp->font_height, PX_AVAILY(textbox));

  /* any space left for enter? */
  if ((fdp->font_height * 2 + 1) > PX_AVAILY(textbox)) {
    WARN("No more room in textbox");
    return false;
  }

  TRACE("Setting cursor to start of new line");

  /* CR */
  textbox->cursor_x = textbox->rect.left;

  /* LF */
  textbox->cursor_y += fdp->font_height + 1;

  return true;
}
/*---------------------------------------------------------------------------*/
static int
draw_character(const struct display_driver * display,
               struct canvas_textbox * textbox,
               const struct font * fdp, const char c)
{
  /* Printable? */
  if ((c >= FONT_FIRST_CH) && (c <= FONT_LAST_CH) &&
      fdp->chars_width[c - FONT_FIRST_CH] > 0) {
    /* Check if CR/LF is needed */
    if ((fdp->chars_width[(uint8_t)c - FONT_FIRST_CH] + 1) > PX_AVAILX(textbox)) {
      if(!draw_crlf(fdp, textbox)) {
        return -1;
      }
    }

    canvas_putch_to_position(display, fdp, textbox, c);
    textbox->cursor_x += fdp->chars_width[c - FONT_FIRST_CH] + 1;
  } else {
    TRACE("Printing other char");
    if (c == ' ') {
      /* space = (height+2)/3 */
      uint8_t spacewidth = (fdp->font_height + 2) / 3;
      if(spacewidth > PX_AVAILX(textbox)){
        if(!draw_crlf(fdp, textbox))
          return -1;
      } else {
        textbox->cursor_x += spacewidth;
      }
    } else if (c == '\r' || c == '\n') {
      draw_crlf(fdp, textbox);
    }
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static inline void
reset_cursor(struct canvas_textbox * textbox)
{
  /* Set cursor position to top left corner of textbox */
  textbox->cursor_x = textbox->rect.left + 2;
  textbox->cursor_y = textbox->rect.top + 2;
}
/*---------------------------------------------------------------------------*/
display_pos_t
canvas_font_height(int font_fd)
{
  struct font * fdp;

  if (!FD_VALID(font_fd)) {
    WARN("Invalid font descriptor");
    return -1;
  }

  fdp = &font_fd_set[font_fd];

  return fdp->font_height;
}
/*---------------------------------------------------------------------------*/
int
canvas_textbox_draw_string(const struct display_driver * display,
            struct canvas_textbox * textbox,
            const int font_fd,
            const char * s)
{
  int total;
  struct font * fdp;

  if (!FD_VALID(font_fd)) {
    WARN("Invalid font descriptor");
    return -1;
  }

  /* Get a pointer to the font */
  fdp = &font_fd_set[font_fd];

  /* Draw textbox */
  canvas_draw_rect(display, &textbox->rect, textbox->border_color, textbox->background_color);

  total = 0;
  while (*s != 0 && draw_character(display, textbox, fdp, *s++) == 1) {
    total++;
  }

  /* Return number of characters written */
  return total;
}
/*---------------------------------------------------------------------------*/
int
canvas_textbox_draw_string_reset(const struct display_driver * display,
                                 struct canvas_textbox * textbox,
                                 const int font_fd,
                                 const char * s)
{
  /* Initialize cursor position */
  reset_cursor(textbox);

  /* Draw the string */
  return canvas_textbox_draw_string(display, textbox, font_fd, s);
}
