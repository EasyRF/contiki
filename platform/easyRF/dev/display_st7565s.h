#ifndef DISPL_ST7565S_H_
#define DISPL_ST7565S_H_

#include <stdbool.h>
#include <stdint.h>

/* Monochrome display */
typedef uint8_t display_color_t;

/* Resolution fits in a uin8t_t */
typedef uint16_t display_pos_t;

/* The display_driver needs a definition for display_color_t */
#include "display_driver.h"

/* The display */
extern const struct display_driver display_st7565s;

/* ST7565s architecture-specific SPI functions that are called by the
   driver and must be implemented by the platform code */
void st7565s_arch_spi_init(void);
uint8_t st7565s_arch_spi_write(uint8_t data);
void st7565s_arch_spi_select(bool is_data);
void st7565s_arch_spi_deselect(void);
void st7565s_arch_set_backlight(bool on);

#endif /* DISPL_ST7565S_H_ */
