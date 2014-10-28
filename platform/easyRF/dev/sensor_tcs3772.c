#include <asf.h>
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "sensor_tcs3772.h"
#include "log.h"

#undef TRACE
#define TRACE(...)

/* Bitshift helper */
#define BM(pos)                 ((uint32_t)1 << pos)

/* Command */
#define COMMAND_BIT             0x80
#define COMMAND_REPEAT          (0x00 << 5)
#define COMMAND_AUTO_INC        (0x01 << 5)
#define COMMAND_SPECIAL         (0x03 << 5)

/* 0x00 Enable register */
#define REG_ENABLE              0x00
#define ENABLE_PEN              BM(2) /* Proximity enable */
#define ENABLE_AEN              BM(1) /* ADC enable */
#define ENABLE_PON              BM(0) /* Power enable */

/* 0x01 RGBC Time register */
#define REG_RGBC_TIME           0x01
#define RGBC_ATIME(ticks)       (256 - (ticks)) /* 2.4ms per tick */

/* 0x02 Proximity time register */
#define REG_PROXMITY_TIME       0x02
#define PROXIMITY_TIME(ticks)   (256 - (ticks)) /* 2.4ms per tick */

/* 0x03 Wait time register */
/* 0x0D Wait time multiply register, multiply by 12 when set */
#define ENABLE_WLONG            BM(1)
/* 0x04 - 0x07 Clear interrupt threshold registers */
/* 0x08 - 0x0B Proximity interrupt threshold registers */
/* 0x0C - Persistence Filter register */
/* 0x0E - Proximity Pulse Count register */
#define REG_PROXIMITY_PULSE_CNT 0x0E


/* 0x0F - Control register */
#define REG_CONTROL             0x0F
#define PDRIVE(strength)        ((strength) << 6)
#define AGAIN(gain)             (gain)

/* Read-only register from here */

/* 0x12 - Device ID register */
#define REG_DEVICE_ID           0x12

/* 0x13 - Status register */
#define REG_STATUS              0x13
#define STATUS_PINT             BM(5)
#define STATUS_AINT             BM(4)
#define STATUS_PVALID           BM(1)
#define STATUS_AVALID           BM(0)

/* 0x14 - 0x1B RGBC channel data registers */
#define REG_RGBC_CDATA          0x14

/* 0x1C - 0x1D Proximity data registers */
#define REG_PROXIMITY_DATA      0x1C

/* The I2C address of the device */
#define SLAVE_ADDRESS           0x29

/* Number of times to retry I2C read/writes */
#define TIMEOUT                 10


struct read_only_regs {
  uint8_t   device_id;
  uint8_t   status;
  uint16_t  cdata;
  uint16_t  rdata;
  uint16_t  gdata;
  uint16_t  bdata;
  uint16_t  pdata;
};


static struct i2c_master_module i2c_master_instance;
static bool sensor_active;
static struct read_only_regs sensor_data;

/*---------------------------------------------------------------------------*/
PROCESS(tcs3772_process, "TCS3772 Process");
/*---------------------------------------------------------------------------*/
static bool
tcs3772_read_reg(uint8_t reg, uint8_t * data)
{
  struct i2c_master_packet packet;
  uint8_t command;
  uint16_t timeout;

  packet.address          = SLAVE_ADDRESS;
  packet.ten_bit_address  = false;
  packet.high_speed       = false;
  packet.hs_master_code   = 0x0;
  packet.data             = &command;
  packet.data_length      = 1;

  /* Setup write buffer */
  command = reg | COMMAND_REPEAT | COMMAND_BIT;

  /* Write buffer to slave until success. */
  timeout = 0;
  while (i2c_master_write_packet_wait_no_stop(&i2c_master_instance, &packet) != STATUS_OK) {
    /* Increment timeout counter and check if timed out. */
    if (timeout++ == TIMEOUT) {
      WARN("timeout");
      return false;
    }
  }

  /* Setup read buffer */
  packet.data = data;
  packet.data_length = sizeof(uint8_t);
  timeout = 0;
  /* Read from slave until success. */
  while (i2c_master_read_packet_wait(&i2c_master_instance, &packet) != STATUS_OK) {
    /* Increment timeout counter and check if timed out. */
    if (timeout++ == TIMEOUT) {
      WARN("timeout");
      return false;
    }
  }

  return true;
}
/*---------------------------------------------------------------------------*/
static bool
tcs3772_write_reg(uint8_t reg, uint8_t data)
{
  struct i2c_master_packet packet;
  uint8_t command[2];
  uint16_t timeout;

  packet.address          = SLAVE_ADDRESS;
  packet.ten_bit_address  = false;
  packet.high_speed       = false;
  packet.hs_master_code   = 0x0;
  packet.data             = command;
  packet.data_length      = 2;

  /* Setup write buffer */
  command[0] = reg | COMMAND_REPEAT | COMMAND_BIT;
  command[1] = data;

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
/*---------------------------------------------------------------------------*/
static bool
tcs3772_read_all(struct read_only_regs * data)
{
  enum status_code ret;
  uint16_t timeout = 0;
  uint8_t command_buffer[1];

  /* Setup buffer and packet struct to start reading at first read-only reg */
  command_buffer[0] = REG_DEVICE_ID | COMMAND_AUTO_INC | COMMAND_BIT;

  struct i2c_master_packet packet;
  packet.address          = SLAVE_ADDRESS;
  packet.ten_bit_address  = false;
  packet.high_speed       = false;
  packet.hs_master_code   = 0x0;
  packet.data             = command_buffer;
  packet.data_length      = 1;

  /* Write buffer to slave until success. */
  while ((ret = i2c_master_write_packet_wait_no_stop(&i2c_master_instance, &packet)) != STATUS_OK) {
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
  packet.data = (uint8_t *)data;
  packet.data_length = sizeof(struct read_only_regs);
  timeout = 0;
  while ((ret = i2c_master_read_packet_wait(&i2c_master_instance, &packet)) != STATUS_OK) {
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

  TRACE("device_id: 0x%02X, status: 0x%02X, cdata: 0x%04X, rdata: 0x%04X, gdata: 0x%04X, bdata: 0x%04X, pdata: 0x%04X\n",
         data->device_id, data->status, data->cdata, data->rdata, data->gdata, data->bdata, data->pdata);

  return true;
}
/*---------------------------------------------------------------------------*/
static void
tcs3772_init(void)
{
  uint8_t id;
  struct i2c_master_config config_i2c_master;

  /* Toggle SCL for some time solves I2C periperhal problem */
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

  if (tcs3772_read_reg(REG_DEVICE_ID, &id)) {
    TRACE("id: 0x%02X\n", id);
  }

  tcs3772_write_reg(REG_CONTROL, AGAIN(0));

  INFO("tcs3772 initialized");
}
/*---------------------------------------------------------------------------*/
static int
activate_sensor(void)
{
  if (!tcs3772_write_reg(REG_ENABLE, ENABLE_PON | ENABLE_AEN | ENABLE_PEN)) {
    return 0;
  }

  sensor_active = true;

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
deactivate_sensor(void)
{
  if (!tcs3772_write_reg(REG_ENABLE, 0x00)) {
    return 0;
  }

  sensor_active = false;

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  uint16_t rgb_max = max(sensor_data.rdata, max(sensor_data.gdata, sensor_data.bdata));

  switch(type) {
  case RED: return sensor_data.rdata;
  case GREEN: return sensor_data.gdata;
  case BLUE: return sensor_data.bdata;
  case CLEAR: return sensor_data.cdata;
  case RED_BYTE: return ((uint32_t)sensor_data.rdata * 255 / rgb_max);
  case GREEN_BYTE: return ((uint32_t)sensor_data.gdata * 255 / rgb_max);
  case BLUE_BYTE: return ((uint32_t)sensor_data.bdata * 255 / rgb_max);
  default:
    WARN("Invalid property");
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  switch(type) {
  case SENSORS_ACTIVE:
  case SENSORS_READY:
    TRACE("sensor active: %d", sensor_active);
    return sensor_active;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int value)
{
  switch(type) {
  case SENSORS_HW_INIT:
    TRACE("**** INIT HW ****");
    tcs3772_init();
    return 1;
  case SENSORS_ACTIVE:
    if(value) {
      TRACE("**** ACTIVATE SENSOR *****");
      activate_sensor();
      process_start(&tcs3772_process, 0);
    } else {
      TRACE("**** DEACTIVATE SENSOR *****");
      deactivate_sensor();
      process_exit(&tcs3772_process);
    }
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
#define TCS3772_READ_INTERVAL (CLOCK_SECOND / 50)

PROCESS_THREAD(tcs3772_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

  etimer_set(&et, TCS3772_READ_INTERVAL);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    tcs3772_read_all(&sensor_data);

    etimer_restart(&et);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(rgbc_sensor, "RGBC Sensor", value, configure, status);
