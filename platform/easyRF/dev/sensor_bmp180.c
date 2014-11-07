#include <asf.h>
#include "contiki.h"
#include "i2c_master_interface.h"
#include "sensor_bmp180.h"
#include "log.h"

#undef TRACE
#define TRACE(...)

/* Bitshift helper */
#define BM(pos)         ((uint32_t)1 << pos)

/* 0xF4 - Measurement Control register */
#define REG_MEAS_CTRL   0xF4
#define MEAS_SCO        BM(5) /* Start of conversion bit */
#define MEAS_OSS        6 /* Shift position for oversampling ratio */

/* Start temperature measurement */
#define MEAS_TEMP       (MEAS_SCO | 0x0E)
#define WAIT_TIME_TEMP  (4500)

/* Start Pressure measurement */
#define MEAS_PRES       0x34
#define MEAS_OSS_ULP    0 /* Ultra low power (1 sample, 4.5ms) */
#define MEAS_OSS_STD    1 /* Standard (2 samples, 7.5ms) */
#define MEAS_OSS_HR     2 /* High Resolution (4 samples, 13.5ms) */
#define MEAS_OSS_UHR    3 /* Ultra High Resolution (8 samples, 25.5ms) */
#define WAIT_TIME_PRESSURE(oss)  ((oss + 1) * 2 * 3000 + 1500)

/* 0xE0 - Soft reset register */
#define REG_SOFT_RST    0xE0  /* Write 0xB6 for POR sequence */

/* 0xD0 - Chip ID register */
#define REG_CHIP_ID     0xD0  /* Value is fixed to 0x55 */

/* 0xF8 - ADC output XLSB */
#define REG_OUT_XLSB    0xF8

/* 0xF7 - ADC output LSB */
#define REG_OUT_LSB     0xF7

/* 0xF6 - ADC output MSB */
#define REG_OUT_MSB     0xF6

/* 0xAA - Start of calibration register map */
#define REG_CALIB       0xAA

/* Register map of calibration values */
struct calibration_regs {
  int16_t ac1;
  int16_t ac2;
  int16_t ac3;
  uint16_t ac4;
  uint16_t ac5;
  uint16_t ac6;
  int16_t b1;
  int16_t b2;
  int16_t mb;
  int16_t mc;
  int16_t md;
};


/* I2C address of the BMP180 sensor */
#define SLAVE_ADDRESS           0x77

/* Update interval */
#define BMP180_READ_INTERVAL    (CLOCK_SECOND * 2)


static bool sensor_active;
static struct calibration_regs calibration_values;
static int32_t temperature;
static int32_t pressure;

/*---------------------------------------------------------------------------*/
PROCESS(bmp180_process, "BMP180 Process");
/*---------------------------------------------------------------------------*/
static bool
bmp180_init(void)
{
  uint8_t cmd, id;

  i2c_master_interface_init();

  cmd = REG_CHIP_ID;
  if (!i2c_master_read_reg(SLAVE_ADDRESS, &cmd, sizeof(cmd), &id, sizeof(id))) {
    return false;
  }

  TRACE("ID: 0x%02X", id);

  cmd = REG_CALIB;
  if (!i2c_master_read_reg(SLAVE_ADDRESS, &cmd, sizeof(cmd), (uint8_t *)&calibration_values, sizeof(calibration_values))) {
    return false;
  }

  calibration_values.ac1 = BE16_TO_CPU(calibration_values.ac1);
  calibration_values.ac2 = BE16_TO_CPU(calibration_values.ac2);
  calibration_values.ac3 = BE16_TO_CPU(calibration_values.ac3);
  calibration_values.ac4 = BE16_TO_CPU(calibration_values.ac4);
  calibration_values.ac5 = BE16_TO_CPU(calibration_values.ac5);
  calibration_values.ac6 = BE16_TO_CPU(calibration_values.ac6);
  calibration_values.b1  = BE16_TO_CPU(calibration_values.b1);
  calibration_values.b2  = BE16_TO_CPU(calibration_values.b2);
  calibration_values.mb  = BE16_TO_CPU(calibration_values.mb);
  calibration_values.mc  = BE16_TO_CPU(calibration_values.mc);
  calibration_values.md  = BE16_TO_CPU(calibration_values.md);

  TRACE("AC1: 0x%04X, AC2: 0x%04X, AC3: 0x%04X, AC4: 0x%04X, AC5: 0x%04X, AC6: 0x%04X, B1: 0x%04X, B2: 0x%04X, MB: 0x%04X, MC: 0x%04X, MD: 0x%04X",
         calibration_values.ac1,
         calibration_values.ac2,
         calibration_values.ac3,
         calibration_values.ac4,
         calibration_values.ac5,
         calibration_values.ac6,
         calibration_values.b1,
         calibration_values.b2,
         calibration_values.mb,
         calibration_values.mc,
         calibration_values.md);

  return true;
}
/*---------------------------------------------------------------------------*/
static bool
read_uncompensated_temperature(uint16_t * temp)
{
  uint8_t read_cmd, write_cmd[2], msb, lsb;

  /* Setup measurement control register for reading temperature */
  write_cmd[0] = REG_MEAS_CTRL;
  write_cmd[1] = MEAS_TEMP;
  if (!i2c_master_write_reg(SLAVE_ADDRESS, write_cmd, sizeof(write_cmd))) {
    return false;
  }

  /* Wait for temperature conversion */
  clock_delay_usec(WAIT_TIME_TEMP);

  /* Read raw temperature from ADC output register */
  read_cmd = REG_OUT_MSB;
  if (!i2c_master_read_reg(SLAVE_ADDRESS, &read_cmd, sizeof(read_cmd), &msb, sizeof(msb))) {
    return false;
  }
  read_cmd = REG_OUT_LSB;
  if (!i2c_master_read_reg(SLAVE_ADDRESS, &read_cmd, sizeof(read_cmd), &lsb, sizeof(lsb))) {
    return false;
  }

  /* Convert bytes to 16-bit value */
  *temp = (((uint16_t)msb << 8) | lsb);

  /* Ok */
  return true;
}
/*---------------------------------------------------------------------------*/
static bool
read_uncompenstated_pressure(uint32_t * pressure, uint8_t oss)
{
  uint8_t read_cmd, write_cmd[2], msb, lsb, xlsb;

  /* Setup meaurement control register for reading pressure */
  write_cmd[0] = REG_MEAS_CTRL;
  write_cmd[1] = MEAS_PRES | (oss << MEAS_OSS);
  if (!i2c_master_write_reg(SLAVE_ADDRESS, write_cmd, sizeof(write_cmd))) {
    return false;
  }

  /* Wait for pressure conversion, depending on number of samples */
  clock_delay_usec(WAIT_TIME_PRESSURE(oss));

  /* Read raw pressure from ADC output register */
  read_cmd = REG_OUT_MSB;
  if (!i2c_master_read_reg(SLAVE_ADDRESS, &read_cmd, sizeof(read_cmd), &msb, sizeof(msb))) {
    return false;
  }
  read_cmd = REG_OUT_LSB;
  if (!i2c_master_read_reg(SLAVE_ADDRESS, &read_cmd, sizeof(read_cmd), &lsb, sizeof(lsb))) {
    return false;
  }
  read_cmd = REG_OUT_XLSB;
  if (!i2c_master_read_reg(SLAVE_ADDRESS, &read_cmd, sizeof(read_cmd), &xlsb, sizeof(xlsb))) {
    return false;
  }

  /* Convert bytes to 24-bit value */
  *pressure = (((uint32_t)msb << 16) | ((uint32_t)lsb << 8) | xlsb);

  /* Correct value based on oversampling ratio */
  *pressure >>= (8 - oss);

  /* Ok */
  return true;
}
/*---------------------------------------------------------------------------*/
static bool
update_values(void)
{
  uint16_t ut;
  uint32_t up;
  int16_t oss;

  int32_t new_temperature, new_pressure;

  int32_t x1, x2, x3, b3, b5, b6, p;
  uint32_t b4, b7;

  oss = MEAS_OSS_ULP;

  if (!read_uncompensated_temperature(&ut)) {
    return false;
  }

  TRACE("ut: %d", ut);

  if (!read_uncompenstated_pressure(&up, oss)) {
    return false;
  }

  TRACE("up: %ld", up);

  x1 = ((ut - calibration_values.ac6) * calibration_values.ac5) >> 15;
  x2 = (calibration_values.mc << 11) / (x1 + calibration_values.md);
  b5 = x1 + x2;
  new_temperature = (b5 + 8) >> 4;

  TRACE("new_temperature = %ld", new_temperature);

  b6 = (b5 - 4000);
  x1 = (calibration_values.b2 * ((b6 * b6) >> 12)) >> 11;
  x2 = (calibration_values.ac2 * b6) >> 11;
  x3 = x1 + x2;
  b3 = (((((long)calibration_values.ac1) * 4 + x3) << oss) + 2) >> 2;
  x1 = (calibration_values.ac3 * b6) >> 13;
  x2 = (calibration_values.b1 * (b6 * b6 >> 12)) >> 16;
  x3 = ((x1 + x2) + 2) >> 2;
  b4 = (calibration_values.ac4 * (unsigned long)(x3 + 32768)) >> 15;
  b7 = ((unsigned long)up - b3) * (50000 >> oss);
  if (b7 < 0x80000000)
    p = (b7 * 2) / b4;
  else
    p = (b7 / b4) * 2;

  x1 = (p >> 8) * (p >> 8);
  x1 = (x1 * 3038) >> 16;
  x2 = (-7357 * p) >> 16;
  new_pressure = p + ((x1 + x2 + 3791) >> 4);

  TRACE("new_pressure = %ld", new_pressure);

  if (new_temperature != temperature ||
      new_pressure != pressure) {
    temperature = new_temperature;
    pressure = new_pressure;
    sensors_changed(&pressure_sensor);
  }

  return true;
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  switch (type) {
  case BMP180_TEMPERATURE: return temperature;
  case BMP180_PRESSURE: return pressure;
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
    bmp180_init();
    return 1;
  case SENSORS_ACTIVE:
    if(value) {
      process_start(&bmp180_process, 0);
      sensor_active = true;
    } else {
      process_exit(&bmp180_process);
      sensor_active = false;
    }
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(bmp180_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

  etimer_set(&et, BMP180_READ_INTERVAL);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    update_values();

    etimer_restart(&et);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(pressure_sensor, "Temperature/Pressure Sensor", value, configure, status);
