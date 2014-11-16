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
#include "log.h"
#include "autofs.h"
#include "canvas_text.h"

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

/* maximum nr of px on a row */
#define FONT_MAX_PX_PER_ROW   (FONT_MAX_WIDTH*FONT_MAX_CHARS)


static int font_file_fd;
static const struct display_driver * curr_display;
static display_pos_t curr_startx, curr_starty, curr_width, curr_height;
static display_pos_t curr_x, curr_y;
static display_pos_t curr_fontsize;
static display_color_t curr_color;
static cfs_offset_t curr_font_dataoffset;
static cfs_offset_t curr_bytes_per_row;

/* The x position of all chars */
static display_pos_t charx[FONT_MAX_CHARS];

/* The width of the chars */
static display_pos_t charw[FONT_MAX_CHARS];

/* Flag indicating font status */
static bool font_loaded;

/*---------------------------------------------------------------------------*/

/* Macro's for calculating pixels left for x and y direction within textbox */
#define PX_AVAILX()   ((curr_startx + curr_width ) - curr_x)
#define PX_AVAILY()   ((curr_starty + curr_height) - curr_y)

/*---------------------------------------------------------------------------*/
static bool
canvas_text_load(void)
{
  struct BMP_HEADER hdr;
  static uint8_t flattened_row[FONT_MAX_PX_PER_ROW/8];
  uint8_t curr_ch = 0;
  display_pos_t tmp_x = 0;
  bool curr_px = 0, last_px = 0;
  display_pos_t x, y;
  uint8_t bitmask = 0, currbyte = 0;
  char ch;

  memset(charx, 0, sizeof(charx));
  memset(charw, 0, sizeof(charw));

  /* Read BMP header */
  if (autofs_read(font_file_fd, (void *)&hdr,
                  sizeof(struct BMP_HEADER)) != sizeof(struct BMP_HEADER)) {
    WARN("Failed to read BMP header");
  }

  /* Log BMP info */
  INFO("BMP size %dx%d bits %d", hdr.width, hdr.height, hdr.bits);

  if(hdr.width>FONT_MAX_PX_PER_ROW){
    WARN("Unable to load font, BPM to wide!");
    return false;
  }
  /* Check color depth */
  if (hdr.bits != 1) {
    WARN("Only monochrome bitmaps are currently supported");
    return false;
  }

  /* Bytes in a row are aligned with 32 bits */
  curr_bytes_per_row = (((uint32_t)hdr.bits * hdr.width + 31) / 32) << 2;
  curr_fontsize = hdr.height;

  TRACE("bytes_per_row: %d", curr_bytes_per_row);

  /* SET seek position to start of image data */
  curr_font_dataoffset = hdr.offset;
  TRACE("font data offset %d", curr_font_dataoffset);

  /* Move pointer in file to start of imagedata */
  autofs_seek(font_file_fd, curr_font_dataoffset, CFS_SEEK_SET);

  /* Fill flattened_row */
  memset(flattened_row,0,sizeof(flattened_row));
  for(y=0;y<hdr.height;y++) {
    for(x=0;x<curr_bytes_per_row;x++) {
      autofs_read(font_file_fd, &ch, 1);
      flattened_row[x] |= (uint8_t) ~ch; /* 0 means black px, 1 means white px, so invert bits */
    }
  }

  /* Walk through flattened_row to fill charx (startposition of char) and charw (width of char) */
  for(x = 0; x < hdr.width; x++) {

    /* Get new byte */
    if ((x%8)==0) {
      bitmask=0x80;
      currbyte=flattened_row[x>>3];
    }

    /* Add a white px to the end to avoid missing the last char */
    curr_px = (currbyte & bitmask) == bitmask;

    if (last_px != curr_px) {
      if (curr_px) { /* start of char */
        tmp_x = x;
      } else {        /* end of char */
        if (curr_ch < FONT_MAX_CHARS) {
          charx[curr_ch] = tmp_x;
          charw[curr_ch] = x - tmp_x;
          TRACE("CHAR %c starts @  %d and is %d wide.",curr_ch + FONT_FIRST_CH, charx[curr_ch], charw[curr_ch]);
          curr_ch++;
        } else {
          WARN("Not enough room for new character");
        }
      }
      last_px = curr_px;
    }
    bitmask >>= 1;
  }

  INFO("Read %d chars (max is %d)", curr_ch, FONT_MAX_CHARS);

  return true;
}
/*---------------------------------------------------------------------------*/
int
canvas_text_init(const struct display_driver * display,
                 const char * filename,
                 display_pos_t startx, display_pos_t starty,
                 display_pos_t width, display_pos_t height,
                 display_color_t textcolor, display_color_t bgcolor)
{
  /* Remember settings in local variables */
  curr_display = display;
  curr_startx  = startx;
  curr_starty  = starty;
  curr_width   = width;
  curr_height  = height;
  curr_color   = textcolor;
  curr_x       = curr_startx;
  curr_y       = curr_starty;

  /* Try opening fontfile */
  font_file_fd = autofs_open(filename, CFS_READ);
  if (font_file_fd < 0) {
    WARN("Could not find %s", filename);
    return -1;
  }

  /* Analyze font image */
  font_loaded = canvas_text_load();

  /* Draw background */
  canvas_fill(curr_display, startx, starty, width, height, bgcolor);

  return font_loaded;
}
/*---------------------------------------------------------------------------*/
static void
canvas_putch_to_position(char ch, display_pos_t displx, display_pos_t disply)
{
  display_pos_t filex, x, y;
  uint8_t currbyte, bitmask;
  uint8_t width=charw[ch-FONT_FIRST_CH];

  for(y = 0; y < curr_fontsize; y++) {

    filex = charx[ch-FONT_FIRST_CH];

    /* Step to right position in file (BMP is always upsidedown, so reversed order) */
    int offset = curr_font_dataoffset + (long)curr_bytes_per_row*(curr_fontsize-1-y) + (filex>>3);

    autofs_seek(font_file_fd, offset, CFS_SEEK_SET);
    autofs_read(font_file_fd, &currbyte, 1);

    bitmask = 0x80 >> (filex % 8);
    for(x = 0; x < width; x++) {

      if ((currbyte & bitmask) != bitmask) {
        curr_display->set_px(displx+x, disply+y, curr_color);
      }

      /* Next sourcepx */
      filex++;
      bitmask >>= 1;

      if(!bitmask) {
        autofs_read(font_file_fd, &currbyte, 1);
        bitmask = 0x80;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static bool
canvas_text_crlf(void)
{
  /* any space left for enter? */
  if ((curr_fontsize+1) > PX_AVAILY()) {
    WARN("No more room in textbox");
    return false;
  }

  TRACE("Setting cursor to start of new line");

  /* CR */
  curr_x = curr_startx;

  /* LF */
  curr_y += curr_fontsize + 1;

  return true;
}
/*---------------------------------------------------------------------------*/
int
canvas_putc(char ch)
{
  /* Do we have a font loaded? */
  if(!font_loaded) {
    WARN("No font loaded");
    return -1;
  }

  /* Printable? */
  if ((ch>=FONT_FIRST_CH) && (ch<=FONT_LAST_CH) && charw[ch-FONT_FIRST_CH]>0) {
    /* CR/LF needed? */
    if ((charw[(uint8_t)ch-FONT_FIRST_CH]+1) > PX_AVAILX()) {
      if(!canvas_text_crlf()) {
        return -1;
      }
    }

    canvas_putch_to_position(ch, curr_x, curr_y);
    curr_x += charw[ch-FONT_FIRST_CH]+1;
  } else {
    TRACE("Printing other char");
    if (ch == ' ') {
      /* space = (height+2)/3 */
      uint8_t spacewidth = (curr_fontsize + 2) / 3;
      if(spacewidth > PX_AVAILX()){
        if(!canvas_text_crlf())
          return -1;
      } else {
        curr_x+=spacewidth;
      }
    } else if (ch == '\r' || ch == '\n') {
      canvas_text_crlf();
    }
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
int
canvas_puts(const char * s)
{
  int total = 0;
  while (*s != 0 && canvas_putc(*s++) == 1) {
    total++;
  }
  return total;
}
