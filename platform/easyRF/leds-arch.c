#include <asf.h>
#include "leds-arch.h"


static unsigned char led_status;

/*---------------------------------------------------------------------------*/
void
leds_arch_init(void)
{
  struct port_config pin_conf;
  port_get_config_defaults(&pin_conf);

  /* Configure LEDs as outputs, turn them off */
  pin_conf.direction = PORT_PIN_DIR_OUTPUT;

  port_pin_set_config(LED_RED_PIN, &pin_conf);
  port_pin_set_output_level(LED_RED_PIN, false);

  port_pin_set_config(LED_GREEN_PIN, &pin_conf);
  port_pin_set_output_level(LED_GREEN_PIN, false);

  port_pin_set_config(LED_BLUE_PIN, &pin_conf);
  port_pin_set_output_level(LED_BLUE_PIN, false);

  port_pin_set_config(LED_WHITE_PIN, &pin_conf);
  port_pin_set_output_level(LED_WHITE_PIN, false);
}
/*---------------------------------------------------------------------------*/
unsigned char
leds_arch_get(void)
{
  return led_status;
}
/*---------------------------------------------------------------------------*/
void
leds_arch_set(unsigned char new_led_status)
{
  led_status = new_led_status;

  port_pin_set_output_level(LED_RED_PIN,   !(led_status & LEDS_RED));
  port_pin_set_output_level(LED_GREEN_PIN, !(led_status & LEDS_GREEN));
  port_pin_set_output_level(LED_BLUE_PIN,  !(led_status & LEDS_BLUE));
  port_pin_set_output_level(LED_WHITE_PIN, !(led_status & LEDS_WHITE));
}
