#include <asf.h>
#include "contiki.h"
#include "i2c_master_interface.h"


struct i2c_master_module i2c_master_instance;

static bool initialized = false;


void
i2c_master_interface_init(void)
{
  if (!initialized) {
    struct i2c_master_config config_i2c_master;

    /* Toggle SCL for some time, this solves I2C periperhal problem */
    struct port_config pin_conf;
    port_get_config_defaults(&pin_conf);
    pin_conf.direction = PORT_PIN_DIR_OUTPUT;
    port_pin_set_config(PIN_PA13, &pin_conf);

    for (int i = 0; i < 1000; i++) {
      port_pin_toggle_output_level(PIN_PA13);
      clock_wait(CLOCK_SECOND / 1000);
    }

    /* Initialize config structure and software module. */
    i2c_master_get_config_defaults(&config_i2c_master);

    /* Change buffer timeout to something longer. */
    config_i2c_master.buffer_timeout = 10000;
    config_i2c_master.baud_rate = 400;
    config_i2c_master.pinmux_pad0 = SENSORS_I2C_SERCOM_PINMUX_PAD0;
    config_i2c_master.pinmux_pad1 = SENSORS_I2C_SERCOM_PINMUX_PAD1;

    /* Initialize and enable device with config. */
    i2c_master_init(&i2c_master_instance, SENSORS_I2C_MODULE, &config_i2c_master);
    i2c_master_enable(&i2c_master_instance);

    initialized = true;
  }
}
