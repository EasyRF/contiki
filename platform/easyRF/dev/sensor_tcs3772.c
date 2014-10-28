#include <asf.h>
#include "sensor_tcs3772.h"
#include "log.h"


#define SLAVE_ADDRESS   0xEE //0x29
#define DATA_LENGTH     10
#define TIMEOUT         1000

static struct i2c_master_module i2c_master_instance;

/*---------------------------------------------------------------------------*/
void
tcs3772_init(void)
{
  struct i2c_master_config config_i2c_master;

  /* Initialize config structure and software module. */
  i2c_master_get_config_defaults(&config_i2c_master);

  /* Change buffer timeout to something longer. */
  config_i2c_master.buffer_timeout = 10000;
  config_i2c_master.baud_rate = 100;
  config_i2c_master.pinmux_pad0 = SENSORS_I2C_SERCOM_PINMUX_PAD0;
  config_i2c_master.pinmux_pad1 = SENSORS_I2C_SERCOM_PINMUX_PAD1;

  /* Initialize and enable device with config. */
  i2c_master_init(&i2c_master_instance, SENSORS_I2C_MODULE, &config_i2c_master);
  i2c_master_enable(&i2c_master_instance);

  INFO("tcs3772 initialized");
}
/*---------------------------------------------------------------------------*/

#define COMMAND_BIT   0x80
#define DEVICE_ID     0x12

void
tcs3772_read_status(void)
{
  enum status_code ret;
  uint16_t timeout = 0;

  uint8_t write_buffer[DATA_LENGTH];
  uint8_t read_buffer[DATA_LENGTH];

  memset(write_buffer, 0, DATA_LENGTH);
  memset(read_buffer, 0, DATA_LENGTH);

  struct i2c_master_packet packet;
  packet.address          = SLAVE_ADDRESS;
  packet.ten_bit_address  = false;
  packet.high_speed       = false;
  packet.hs_master_code   = 0x0;

  /* Setup buffer and packet struct to read device id */
  write_buffer[0] = DEVICE_ID | COMMAND_BIT;
  packet.data_length      = 1;
  packet.data             = write_buffer;
  /* Write buffer to slave until success. */
  while ((ret = i2c_master_write_packet_wait_no_stop(&i2c_master_instance, &packet)) !=
      STATUS_OK) {
    /* Increment timeout counter and check if timed out. */
    if (timeout++ == TIMEOUT) {
      WARN("timeout");
      break;
    } else {
      if (ret != 0x12) {
        TRACE("write attempt failed with 0x%02X, retrying", ret);
      }
    }
  }

  /* Read from slave until success. */
  packet.data = read_buffer;
  packet.data_length = 1;
  timeout = 0;
  while ((ret = i2c_master_read_packet_wait(&i2c_master_instance, &packet)) !=
      STATUS_OK) {
    /* Increment timeout counter and check if timed out. */
    if (timeout++ == TIMEOUT) {
      WARN("timeout");
      break;
    } else {
      if (ret != 0x12) {
        TRACE("read attempt failed with 0x%02X, retrying", ret);
      }
    }
  }

  printf("Response: ");
  for (int i = 0; i < DATA_LENGTH; i++) {
    printf("%02X ", read_buffer[i]);
  }
  printf("\n");
}
