#include <asf.h>
#include "contiki.h"
#include "i2c_master_interface.h"
#include "log.h"


/* Number of times to retry I2C read/writes */
#define TIMEOUT                 100


struct i2c_master_module i2c_master_instance;

static bool initialized = false;

/*---------------------------------------------------------------------------*/
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
    config_i2c_master.baud_rate = 400;
    config_i2c_master.pinmux_pad0 = SENSORS_I2C_SERCOM_PINMUX_PAD0;
    config_i2c_master.pinmux_pad1 = SENSORS_I2C_SERCOM_PINMUX_PAD1;

    /* Initialize and enable device with config. */
    i2c_master_init(&i2c_master_instance, SENSORS_I2C_MODULE, &config_i2c_master);
    i2c_master_enable(&i2c_master_instance);

    initialized = true;
  }
}
/*---------------------------------------------------------------------------*/
bool
i2c_master_read_reg(uint8_t slave_addr,
                    uint8_t * tx_data, uint8_t tx_len,
                    uint8_t * rx_data, uint8_t rx_len)
{
  struct i2c_master_packet packet;
  uint16_t timeout;

  /* Setup packet struct */
  packet.address          = slave_addr;
  packet.ten_bit_address  = false;
  packet.high_speed       = false;
  packet.hs_master_code   = 0x0;

  /* Setup write buffer and reset timeout */
  packet.data = tx_data;
  packet.data_length = tx_len;
  timeout = 0;
  /* Write buffer to slave until success */
  while (i2c_master_write_packet_wait_no_stop(&i2c_master_instance, &packet) != STATUS_OK) {
    /* Increment timeout counter and check if timed out. */
    if (timeout++ == TIMEOUT) {
      WARN("write timeout");
      return false;
    }
  }

  /* Setup read buffer and reset timeout */
  packet.data = rx_data;
  packet.data_length = rx_len;
  timeout = 0;
  /* Read from slave until success. */
  while (i2c_master_read_packet_wait(&i2c_master_instance, &packet) != STATUS_OK) {
    /* Increment timeout counter and check if timed out. */
    if (timeout++ == TIMEOUT) {
      WARN("read timeout");
      return false;
    }
  }

  return true;
}
/*---------------------------------------------------------------------------*/
bool
i2c_master_write_reg(uint8_t slave_addr, uint8_t * data, uint8_t len)
{
  struct i2c_master_packet packet;
  uint16_t timeout;

  packet.address          = slave_addr;
  packet.ten_bit_address  = false;
  packet.high_speed       = false;
  packet.hs_master_code   = 0x0;
  packet.data             = data;
  packet.data_length      = len;

  /* Write buffer to slave until success. */
  timeout = 0;
  while (i2c_master_write_packet_wait(&i2c_master_instance, &packet) != STATUS_OK) {
    /* Increment timeout counter and check if timed out. */
    if (timeout++ == TIMEOUT) {
      WARN("timeout");
      return false;
    }
  }

  return true;
}
