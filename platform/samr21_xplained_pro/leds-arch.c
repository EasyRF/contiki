#include <asf.h>
#include "leds-arch.h"


static unsigned char led_status;


void
leds_arch_init(void)
{
  struct port_config pin_conf;
  port_get_config_defaults(&pin_conf);

  /* Configure LEDs as outputs, turn them off */
  pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
  port_pin_set_config(LED_0_PIN, &pin_conf);
  port_pin_set_output_level(LED_0_PIN, LED_0_INACTIVE);
}


unsigned char
leds_arch_get(void)
{
  return led_status;
}


void
leds_arch_set(unsigned char new_led_status)
{
  led_status = new_led_status;
  if (led_status & LEDS_GREEN) {
    port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
  } else {
    port_pin_set_output_level(LED_0_PIN, LED_0_INACTIVE);
  }
}
