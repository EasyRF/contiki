#include <asf.h>
#include "dev/button-sensor.h"
#include "log.h"

/*---------------------------------------------------------------------------*/
static struct timer debouncetimer;
static uint8_t interrupt_enabled;
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  int level = port_pin_get_input_level(BUTTON_0_PIN);
  return !level; /* Pressed returns 1, released returns 0 */
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  switch(type) {
  case SENSORS_ACTIVE:
  case SENSORS_READY:
    return interrupt_enabled;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void extint_detection_callback(void)
{
  if(timer_expired(&debouncetimer)) {
    timer_set(&debouncetimer, CLOCK_SECOND / 8);
    sensors_changed(&button_sensor);
  }
}
/*---------------------------------------------------------------------------*/
static void
configure_button(void)
{
  /* Set buttons as inputs */
  struct port_config pin_conf;
  port_get_config_defaults(&pin_conf);
  pin_conf.direction  = PORT_PIN_DIR_INPUT;
  pin_conf.input_pull = PORT_PIN_PULL_UP;
  port_pin_set_config(BUTTON_0_PIN, &pin_conf);

  /* Configure interrupt */
  struct extint_chan_conf config_extint_chan;
  extint_chan_get_config_defaults(&config_extint_chan);
  config_extint_chan.gpio_pin           = BUTTON_0_EIC_PIN;
  config_extint_chan.gpio_pin_mux       = BUTTON_0_EIC_MUX;
  config_extint_chan.gpio_pin_pull      = EXTINT_PULL_UP;
  config_extint_chan.detection_criteria = EXTINT_DETECT_BOTH;
  extint_chan_set_config(BUTTON_0_EIC_LINE, &config_extint_chan);

  /* Register interrupt callbacks */
  extint_register_callback(extint_detection_callback,
                           BUTTON_0_EIC_LINE,
                           EXTINT_CALLBACK_TYPE_DETECT);
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int value)
{
  switch(type) {
  case SENSORS_HW_INIT:
    configure_button();
    return 1;
  case SENSORS_ACTIVE:
    if(value) {
      if(!interrupt_enabled) {
        timer_set(&debouncetimer, 0);
        extint_chan_enable_callback(BUTTON_0_EIC_LINE,
                                    EXTINT_CALLBACK_TYPE_DETECT);
        interrupt_enabled = 1;
      }
    } else {
      extint_chan_disable_callback(BUTTON_0_EIC_LINE,
                                   EXTINT_CALLBACK_TYPE_DETECT);
      interrupt_enabled = 0;
    }
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(button_sensor, "Button Sensor", value, configure, status);
