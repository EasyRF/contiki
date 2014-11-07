#ifndef __DISPL_H__
#define __DISPL_H__


#define DISPL_DATA_SIZE_X   ((1<<DISPL_X_SIZE_2LOG)*DISPL_BITS_PER_PX/8)
#define DISPL_DATA_SIZE_Y   (1<<DISPL_Y_SIZE_2LOG)
#define DISPL_DATA_SIZE     (DISPL_DATA_SIZE_X*DISPL_DATA_SIZE_Y)

/* Generic display driver return values. */
enum {
  DISPL_OK,
  DISPL_ERR,
  DISPL_ERR_NOTFOUND,
};

struct display_driver {

  /** Initialize display */
  int (* init)(void);

  /** Switch on the display. */
  int (* on)(void);

  /** Switch off the display. */
  int (* off)(void);

  /** Stop updating on the display. */
  int (* freeze)(bool freeze);

  /** Clear display. */
  int (* clear)(void);

  /** Force a complete refresh the display. */
  int (* force_update)(void);

  /** Select the layer that is updated. */
  int (* layer_select)(short layerid);

  /** Select the layer that is showed. */
  int (* layer_activate)(short layerid);

  /** Select the layer that is showed. */
  int (* layer_cpy)(short to, short from);

  /** Change pixel */
  int (* set_px)(int x, int y, COLOR_t color);

  /** Change the contrast (0=0%, 100=100%). */
  int (* set_contrast)(short contrast);

  /** Toggle backlight */
  int (* set_backlight)(bool on);

};

#endif /* DISPL_H_ */
