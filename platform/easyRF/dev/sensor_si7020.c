#include <asf.h>
#include "contiki.h"
#include "i2c_master_interface.h"
#include "sensor_si7020.h"
#include "log.h"


#undef TRACE
#define TRACE(...)


/* Measure Relative Humidity, Hold Master Mode */
#define REG_MEAS_HUMID_HOLD       0xE5
/* Measure Relative Humidity, NO Hold Master Mode */
#define REG_MEAS_HUMID_NO_HOLD    0xF5
/* Measure Temperature, Hold Master Mode */
#define REG_MEAS_TEMP_HOLD        0xE3
/* Measure Temperature, NO Hold Master Mode */
#define REG_MEAS_TEMP_NO_HOLD     0xF3
/* Read temperature from previous RH measurement */
#define REG_READ_PREV_TEMP        0xE0
/* Software reset */
#define REG_RESET                 0xFE
/* Write RH/T User Register 1 */
#define REG_USER_WRITE            0xE6
/* Read RH/T User Register 1 */
#define REG_USER_READ             0xE7
/* Read ID (byte 1) */
#define REG_CHIP_ID_1             0xFA,0x0F
/* Read ID (byte 2) */
#define REG_CHIP_ID_2             0xFC,0xC9
/* Read Firmware Revision */
#define REG_FW_REV                0x84,0xB8


/* I2C address of the BMP180 sensor */
#define SLAVE_ADDRESS             0x40

/* The read interval */
#define SI7020_READ_INTERVAL      (CLOCK_SECOND * 2)


static bool sensor_active;
static int relative_humidity;
static int temperature;

/*---------------------------------------------------------------------------*/
PROCESS(si7020_process, "Si7020 Process");
/*---------------------------------------------------------------------------*/
static bool
read_serial_id(void)
{
  uint8_t id_command_a[2] = {REG_CHIP_ID_1};
  uint8_t id_command_b[2] = {REG_CHIP_ID_2};
  uint8_t sna[8];
  uint8_t snb[6];

  /* Read first part of SN */
  if (!i2c_master_read_reg(SLAVE_ADDRESS, id_command_a, sizeof(id_command_a), sna, sizeof(sna))) {
    return false;
  }

  /* Read second part of SN */
  if (!i2c_master_read_reg(SLAVE_ADDRESS, id_command_b, sizeof(id_command_b), sna, sizeof(snb))) {
    return false;
  }

  /* Log */
  TRACE("0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X",
        sna[0], sna[2], sna[4], sna[6], snb[0], snb[1], snb[3], snb[4]);

  /* Ok */
  return true;
}
/*---------------------------------------------------------------------------*/
static bool
si7020_init(void)
{
  i2c_master_interface_init();

  if (!read_serial_id()) {
    return false;
  }

  INFO("Si7020 initialized");

  return true;
}
/*---------------------------------------------------------------------------*/
static bool
update_values(void)
{
  uint8_t cmd;
  uint16_t rh, temp;
  int new_relative_humidity, new_temperature;

  /* Setup command */
  cmd = REG_MEAS_HUMID_HOLD;
  /* Start humidity conversion and hold for the response */
  if (!i2c_master_read_reg(SLAVE_ADDRESS, &cmd, sizeof(cmd), (uint8_t *)&rh, sizeof(rh))) {
    return false;
  }

  /* Response is Big-Endian, convert to CPU's endianess */
  rh = BE16_TO_CPU(rh);

  /* Convert raw rh value to % RH */
  new_relative_humidity = ((125 * (uint32_t)rh) >> 16) - 6;

  /* Log */
  TRACE("new_relative_humidity: %d", new_relative_humidity);

  /* Setup command */
  cmd = REG_READ_PREV_TEMP;
  /* Read temperature of above RH conversion */
  if (!i2c_master_read_reg(SLAVE_ADDRESS, &cmd, sizeof(cmd), (uint8_t *)&temp, sizeof(temp))) {
    return false;
  }

  /* Response is Big-Endian, convert to CPU's endianess */
  temp = BE16_TO_CPU(temp);

  /* Convert raw temp value to degrees Celcius */
  new_temperature = ((176 * (int32_t)temp) >> 16) - 47;

  /* Log */
  TRACE("new_temperature: %d", new_temperature);

  if (new_relative_humidity != relative_humidity ||
      new_temperature != temperature) {
    relative_humidity = new_relative_humidity;
    temperature = new_temperature;
    sensors_changed(&rh_sensor);
  }

  /* Ok */
  return true;
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  switch (type) {
  case SI7020_HUMIDITY: return relative_humidity;
  case SI7020_TEMPERATURE: return temperature;
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
    if (si7020_init()) {
      return 1;
    }
  case SENSORS_ACTIVE:
    if(value) {
      process_start(&si7020_process, 0);
      sensor_active = true;
    } else {
      process_exit(&si7020_process);
      sensor_active = false;
    }
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(si7020_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

  etimer_set(&et, SI7020_READ_INTERVAL);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    update_values();

    etimer_restart(&et);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(rh_sensor, "Humidity/Temperature Sensor", value, configure, status);
