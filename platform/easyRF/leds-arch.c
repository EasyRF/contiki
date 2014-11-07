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
