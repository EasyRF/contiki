#ifndef DISPL_ST7565S_H_
#define DISPL_ST7565S_H_

#include <stdint.h>
#include <stdbool.h>

#ifndef DISPL_LAYER_CNT
#define DISPL_LAYER_CNT         1       /* 1 layer is default   */
#endif
#define DISPL_BITS_PER_PX       1       /* 1 BIT PER PIXEL      */

/* For rapid row/column jumping we must be able to use shift ( << or >> ) instructions  */
#define DISPL_X_SIZE_2LOG       7       /* 128 pixels wide -> log(128)/log2 = 7      */
#define DISPL_Y_SIZE_2LOG       6       /* 64  pixels wide -> log(64 )/log2 = 6      */

typedef bool COLOR_t;                   /* Monochrome display   */

#include "display_driver.h"

extern const struct display_driver displ_drv_st7565s;

/* ST7565s architecture-specific SPI functions that are called by the
   driver and must be implemented by the platform code */

void st7565s_arch_spi_init(void);
uint8_t st7565s_arch_spi_write(uint8_t data);
void st7565s_arch_spi_select(bool is_data);
void st7565s_arch_spi_deselect(void);
void st7565s_arch_set_backlight(bool on);

#endif /* DISPL_ST7565S_H_ */
