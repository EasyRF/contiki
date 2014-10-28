#include <asf.h>
#include "lcd_34fpc.h"


void
lcd_init(void)
{
  /* Configure SPI CS pin for LCD */
  struct port_config pin_conf;
  port_get_config_defaults(&pin_conf);
  pin_conf.direction = PORT_PIN_DIR_OUTPUT;
  port_pin_set_config(DISPLAY_CS, &pin_conf);

  /* Turn of CS of LCD */
  port_pin_set_output_level(DISPLAY_CS, true);
}
