#ifndef __DISPL_H__
#define __DISPL_H__

#include "contiki.h"
#include "display-conf.h"


/* Display get/set parameter types */
typedef clock_time_t display_value_t;
typedef unsigned display_param_t;


/* Display properties */
enum {
  DISPLAY_WIDTH,
  DISPLAY_HEIGHT,
  DISPLAY_COLOR_DEPTH,
  DISPLAY_CONTRAST,
  DISPLAY_BACKLIGHT,
  DISPLAY_UPDATE_INTERVAL,
  DISPLAY_FLIP_X,
  DISPLAY_FLIP_Y,
  DISPLAY_FREEZE,
  DISPLAY_ACTIVE_LAYER
};


/* Display return values when setting or getting display parameters. */
typedef enum {
  DISPLAY_RESULT_OK,
  DISPLAY_RESULT_NOT_SUPPORTED,
  DISPLAY_RESULT_INVALID_VALUE,
  DISPLAY_RESULT_ERROR
} display_result_t;


/* Display driver API */
struct display_driver {

  /** Initialize display */
  int (* init)(void);

  /** Switch on the display. */
  int (* on)(void);

  /** Switch off the display. */
  int (* off)(void);

  /** Clear display. */
  int (* clear)(void);

  /** Force a complete refresh the display. */
  int (* force_update)(void);

  /** Change pixel */
  int (* set_px)(display_pos_t x, display_pos_t y, display_color_t color);

  /** Invert pixel */
  int (* invert_px)(display_pos_t x, display_pos_t y);

  /** Set value */
  display_result_t (* set_value)(display_param_t key, display_value_t value);

  /** Get value */
  display_result_t (* get_value)(display_param_t key, display_value_t * value);

#if 0
  /** Select the layer that is updated. */
  int (* layer_select)(short layerid);

  /** Select the layer that is showed. */
  int (* layer_activate)(short layerid);

  /** Select the layer that is showed. */
  int (* layer_cpy)(short to, short from);
#endif

};

#endif /* DISPL_H_ */
