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

#include "contiki.h"
#include "log.h"
#include "display_st7565s.h"


/* Dislay specific constants */
#define ST7565S_CMD                         false
#define ST7565S_DATA                        true
#define ST7565S_BITS_PER_PX                 1       /* 1 BIT PER PIXEL      */
#define ST7565S_X_SIZE_2LOG                 7       /* 128 pixels wide -> log(128)/log2 = 7      */
#define ST7565S_Y_SIZE_2LOG                 6       /* 64  pixels wide -> log(64 )/log2 = 6      */
#define ST7565S_WIDTH                       (1 << ST7565S_X_SIZE_2LOG)
#define ST7565S_HEIGHT                      (1 << ST7565S_Y_SIZE_2LOG)
#define ST7565S_DATA_SIZE_X                 (ST7565S_WIDTH * ST7565S_BITS_PER_PX / 8)
#define ST7565S_DATA_SIZE_Y                 ST7565S_HEIGHT
#define ST7565S_DATA_SIZE                   (ST7565S_DATA_SIZE_X * ST7565S_DATA_SIZE_Y)
#define ST7565S_ERR                         -1

/* Layer configuration */
#ifndef ST7565S_LAYER_CNT
#define ST7565S_LAYER_CNT                   1 /* 1 layer is default   */
#endif

/* Display commands */
#define ST7565S_CMD_ONOFF                   0xAE /* b0 = on/off */
#define ST7565S_CMD_START_LINE              0x40 /* b0-b5 = line 0-63 */
#define ST7565S_CMD_PAGE_ADDR               0xB0
#define ST7565S_CMD_COLUMN_ADDR_LOWBYTE     0x00
#define ST7565S_CMD_COLUMN_ADDR_HIGHBYTE    0x10
#define ST7565S_CMD_ADC_SEL                 0xA0 /* b0 = normal/reverse */
#define ST7565S_CMD_NORMAL_REVERSE          0xA6 /* b0 = reverse/normal */
#define ST7565S_CMD_ALLPOINTS_ONOFF         0xA4 /* b0 = on/off */
#define ST7565S_CMD_BIAS                    0xA2 /* b0 = bias1/bias2 */
#define ST7565S_CMD_RESET                   0xE2
#define ST7565S_CMD_COMMON_OUT_MODE_SEL     0xC0 /* b3 = normal/reverse, b0-b2 = mode*/
#define ST7565S_CMD_POWER                   0x28 /* b0 = voltage follower, b1=voltage regulator, b2=booster */
#define ST7565S_CMD_V_RATIO                 0x20 /* b0-b2 = ratio */
#define ST7565S_CMD_CONTRAST                0x81 /* DOUBLE BYTE CMD! second byte is contrast 0-63 */
#define ST7565S_CMD_CURSOR                  0xAC /* DOUBLE BYTE CMD! b0 = on/off, second byte b0-b1=OFF/1sblink/0.5sblink/continious */

/* Macro for changing cursor position */
#define ST7565S_SET_COLUMN(ca)    do{ write_command(ST7565S_CMD_COLUMN_ADDR_LOWBYTE|(ca&0x0F));write_command(ST7565S_CMD_COLUMN_ADDR_HIGHBYTE|((ca>>4)&0x0F)); }while(0)
#define ST7565S_SET_PAGE(pa)      write_command(ST7565S_CMD_PAGE_ADDR|(pa&0x0F));
#define ST7565S_SET_POS(ca,pa)    do{ ST7565S_SET_COLUMN(ca);ST7565S_SET_PAGE(pa); }while(0)

/* Default update interval */
#define ST7565S_DEFAULT_UPDATE_INTERVAL   (CLOCK_SECOND / 10)

/* Initialized flag */
static bool initialized;

/* Freezes the display */
static bool freezed;

/* Something is changed in the graphical data */
static bool needs_update;

/* Pixel data of all layers */
static unsigned char data[ST7565S_LAYER_CNT][ST7565S_DATA_SIZE];

/* current activated layer */
static short selected_layer;

/* current displayed layer */
static short active_layer;

/* Contrast value 0-100 */
static uint8_t contrast;

/* Backlight */
static bool backlight_on;

/* Flip X coordinate */
static bool flip_x;

/* Flip Y coordinate */
static bool flip_y;

/* Timer for regular screen update */
static struct etimer update_timer;

/* Easy access pointers */
static unsigned char * pdata_start;
static unsigned char * pdata_end;
static unsigned char * pdata_active_start;
static unsigned char * pdata_active_end;

/*---------------------------------------------------------------------------*/
PROCESS(st7565s_process, "ST7565s display process");
/*---------------------------------------------------------------------------*/
static inline unsigned char *
get_buffer_at_position(display_pos_t x, display_pos_t y)
{
  return (unsigned char *)( (int)pdata_start + ( (x << (ST7565S_Y_SIZE_2LOG - 3)) + (y >> 3) ) );
}
/*---------------------------------------------------------------------------*/
static void
write_command(uint8_t cmd)
{
  st7565s_arch_spi_select(ST7565S_CMD);
  st7565s_arch_spi_write(cmd);
  st7565s_arch_spi_deselect();
}
/*---------------------------------------------------------------------------*/
static void
write_data(uint8_t data)
{
  st7565s_arch_spi_select(ST7565S_DATA);
  st7565s_arch_spi_write(data);
  st7565s_arch_spi_deselect();
}
/*---------------------------------------------------------------------------*/
static int
set_defaults(void)
{
  active_layer   = 0;
  contrast       = 37;
  backlight_on   = false;
  freezed        = 0;
  selected_layer = 0;
  needs_update   = true;

  etimer_set(&update_timer, ST7565S_DEFAULT_UPDATE_INTERVAL);

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  /* Power control */
  write_command(ST7565S_CMD_POWER | 7);

  /* Enable display */
  write_command(ST7565S_CMD_ONOFF | 1);

  /* Start auto update process */
  process_start(&st7565s_process, 0);

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  /* End auto update process */
  process_exit(&st7565s_process);

  /* Power control */
  write_command(ST7565S_CMD_POWER | 0);

  /* Enable display */
  write_command(ST7565S_CMD_ONOFF | 0);

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
clear(void)
{
  memset(data, 0, sizeof(data));
  needs_update = true;

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
force_update(void)
{
  int i;

  if(!initialized) {
    return ST7565S_ERR;
  }

  if (freezed) {
    return 0;
  }

  for (i = 0; i < ST7565S_DATA_SIZE; i++) {
    if (i % ST7565S_WIDTH == 0) {
      /* New row, skip first 4 columns since they are not used by display */
      ST7565S_SET_POS(4, i >> ST7565S_X_SIZE_2LOG);
    }
    write_data(pdata_active_start[((i%128)<<(ST7565S_Y_SIZE_2LOG-3)) + (i>>ST7565S_X_SIZE_2LOG)]);
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
#if 0
static int
layer_select(short layerid)
{
  if(layerid>=ST7565S_LAYER_CNT)
    return ST7565S_ERR;
  selected_layer = layerid;
  pdata_start    = data[layerid];
  pdata_end      = pdata_start + ST7565S_DATA_SIZE;
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
layer_activate(short layerid)
{
  if(layerid>=ST7565S_LAYER_CNT)
    return ST7565S_ERR;
  active_layer       = layerid;
  pdata_active_start = data[layerid];
  pdata_active_end   = pdata_start + ST7565S_DATA_SIZE;
  needs_update = true;
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
layer_cpy(short to, short from)
{
  /* Not implemented */
  return 0;
}
#endif
/*---------------------------------------------------------------------------*/
static int
set_px(display_pos_t x, display_pos_t y, display_color_t color)
{
  unsigned char * pos;
  unsigned char mask;

//  TRACE("x = %d, y = %d, color = %d", x, y, color);

  /* Check state and input parameters */
  if (!initialized ||
      x >= ST7565S_WIDTH ||
      y >= ST7565S_HEIGHT) {
    return ST7565S_ERR;
  }

  if (flip_x) {
    x = ST7565S_WIDTH  - 1 - x;
  }

  if (flip_y) {
    y = ST7565S_HEIGHT - 1 - y;
  }

  /* Get pointer to pixel position in buffer */
  pos = get_buffer_at_position(x, y);

  /* Prepare mask */
  mask = 1 << (y % 8);

  /* Set or clear a pixel */
  if (color == DISPLAY_COLOR_BLACK) {
    *pos |= mask;
  } else if (color == DISPLAY_COLOR_WHITE) {
    *pos &= ~mask;
  } else if (color == DISPLAY_COLOR_TRANSPARENT) {
    /* Do nothing */
  } else {
    WARN("Unknown color");
  }

  /* Mark for update */
  needs_update = true;

  /* Ok */
  return 0;
}
/*---------------------------------------------------------------------------*/
static display_color_t
get_px(display_pos_t x, display_pos_t y)
{
  unsigned char * pos;
  unsigned char mask;

  /* Check state and input parameters */
  if (!initialized ||
      x >= ST7565S_WIDTH ||
      y >= ST7565S_HEIGHT) {
    return ST7565S_ERR;
  }

  if (flip_x) {
    x = ST7565S_WIDTH  - 1 - x;
  }

  if (flip_y) {
    y = ST7565S_HEIGHT - 1 - y;
  }

  /* Get pointer to pixel position in buffer */
  pos = get_buffer_at_position(x, y);

  /* Prepare mask */
  mask = 1 << (y % 8);

  /* Return pixel value */
  return (*pos & mask);
}
/*---------------------------------------------------------------------------*/
static int
invert_px(display_pos_t x, display_pos_t y)
{
  return set_px(x, y, get_px(x, y) == DISPLAY_COLOR_WHITE ?
                  DISPLAY_COLOR_BLACK : DISPLAY_COLOR_WHITE);
}
/*---------------------------------------------------------------------------*/
static int
set_contrast(short c)
{
  contrast = c;
  write_command(ST7565S_CMD_CONTRAST);
  write_command(contrast);
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
set_backlight(bool on)
{
  backlight_on = on;
  st7565s_arch_set_backlight(on);
  return 0;
}
/*---------------------------------------------------------------------------*/
static display_result_t
set_value(display_param_t key, display_value_t value)
{
  switch (key) {
  case DISPLAY_CONTRAST:
    set_contrast(value);
    break;
  case DISPLAY_BACKLIGHT:
    set_backlight(value);
    break;
  case DISPLAY_UPDATE_INTERVAL:
    etimer_set(&update_timer, value);
    break;
  case DISPLAY_FREEZE:
    freezed = value;
    break;
  case DISPLAY_FLIP_X:
    flip_x = value;
    break;
  case DISPLAY_FLIP_Y:
    flip_y = value;
    break;
  case DISPLAY_ACTIVE_LAYER:
  case DISPLAY_COLOR_DEPTH:
  case DISPLAY_WIDTH:
  case DISPLAY_HEIGHT:
  default:
    return DISPLAY_RESULT_NOT_SUPPORTED;
  }
  return DISPLAY_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static display_result_t
get_value(display_param_t key, display_value_t * value)
{
  switch (key) {
  case DISPLAY_WIDTH:
    *value = ST7565S_WIDTH;
    break;
  case DISPLAY_HEIGHT:
    *value = ST7565S_HEIGHT;
    break;
  case DISPLAY_COLOR_DEPTH:
    *value = ST7565S_BITS_PER_PX;
    break;
  case DISPLAY_CONTRAST:
    *value = contrast;
    break;
  case DISPLAY_BACKLIGHT:
    *value = backlight_on;
    break;
  case DISPLAY_UPDATE_INTERVAL:
    *value = update_timer.timer.interval;
    break;
  case DISPLAY_FREEZE:
    *value = freezed;
    break;
  case DISPLAY_FLIP_X:
    *value = flip_x;
    break;
  case DISPLAY_FLIP_Y:
    *value = flip_y;
    break;
  case DISPLAY_ACTIVE_LAYER:
  default:
    return DISPLAY_RESULT_NOT_SUPPORTED;
  }
  return DISPLAY_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static int
init(void)
{
  set_defaults();

  st7565s_arch_spi_init();

  write_command(ST7565S_CMD_BIAS|0);                            /* BIAS RATIO High */
  write_command(ST7565S_CMD_ADC_SEL|1);                         /* Normal ADC Segment driver Direction */
  write_command(ST7565S_CMD_COMMON_OUT_MODE_SEL|0);             /* Normal Common Output Mode Select */
  write_command(ST7565S_CMD_V_RATIO|5);                         /* V0 Voltage Internal Resistor Ratio Set 0-7 */

  clear();
  set_contrast(contrast);
  set_backlight(backlight_on);

  pdata_start           = data[selected_layer];
  pdata_end             = pdata_start + ST7565S_DATA_SIZE;
  pdata_active_start    = data[active_layer];
  pdata_active_end      = pdata_active_start + ST7565S_DATA_SIZE;

  initialized = true;

  force_update();

  return 0;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(st7565s_process, ev, data)
{
  PROCESS_BEGIN();

  while(1) {
    etimer_restart(&update_timer);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&update_timer));

    if (needs_update) {
      needs_update = false;
      force_update();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
const struct display_driver display_st7565s =
{
  init,
  on,
  off,
  clear,
  force_update,
  set_px,
  invert_px,
  set_value,
  get_value
#if 0
  layer_select,
  layer_activate,
  layer_cpy
#endif
};
/*---------------------------------------------------------------------------*/
