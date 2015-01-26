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
#include "contiki.h"
#include "i2c_master_interface.h"
#include "sensor_tcs3772.h"
#include "log.h"

#undef TRACE
#define TRACE(...)

/* Filter sensor values */
#ifndef RGBCP_FILTER_ENABLE
#define SENSOR_FILTER_ENABLE     0
#endif
/* High value means less noise and slower detection */
#ifndef RGBCP_FILTER_FACTOR
#define SENSOR_FILTER_FACTOR     1
#endif
/* Nr. of proximity pulses generated (0..255) */
#ifndef PROX_PULSES
#define PROX_PULSES              20
#endif
/* Integration time of proximity sensor in steps of 2.4ms (1..256) */
#ifndef PROX_INTEGRATION_TIME
#define PROX_INTEGRATION_TIME    10
#endif
/* Proximity LED Drive Strength (0=100mA, 1=50mA, 2=25mA, 3=12.5mA) */
#ifndef PROX_LED_DRIVE_STRENGTH
#define PROX_LED_DRIVE_STRENGTH  0
#endif
/* RGBC Gain Control (0=1x, 1=4x, 2=16x, 3=64x) */
#ifndef RGBC_GAIN
#define RGBC_GAIN                0
#endif
/* Integration time of RGBC sensor in steps of 2.4ms (1..256) */
#ifndef RGBC_INTEGRATION_TIME
#define RGBC_INTEGRATION_TIME    10
#endif


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

/* Update interval */
#define TCS3772_DEFAULT_READ_INTERVAL   (CLOCK_SECOND / 2)

/* Nr. of sensors in the chip */
#define NR_OF_SENSORS          5

struct status_regs {
  uint8_t   status;
  uint16_t  value[NR_OF_SENSORS];
} __attribute__ ((packed));


static bool sensor_active;
static struct status_regs sensor_data;
static struct etimer read_timer;

/*---------------------------------------------------------------------------*/
PROCESS(tcs3772_process, "TCS3772 Process");
/*---------------------------------------------------------------------------*/
static bool
update_values(void)
{

  static bool first_time=1;
  struct status_regs tmp_sensor_data;
  struct status_regs new_sensor_data;
  uint8_t cmd;

  /* Get sensor values from chip */
  cmd = REG_STATUS | COMMAND_AUTO_INC | COMMAND_BIT;
  if (!i2c_master_read_reg(SLAVE_ADDRESS, &cmd, sizeof(cmd),
                           (uint8_t *)&tmp_sensor_data, sizeof(tmp_sensor_data))) {
    return false;
  }

  /* Print new values */
  TRACE("NEW: status: 0x%02X, cdata: 0x%04X, rdata: 0x%04X, gdata: 0x%04X, bdata: 0x%04X, pdata: 0x%04X\n",
        tmp_sensor_data.status, tmp_sensor_data.value[0], tmp_sensor_data.value[1], tmp_sensor_data.value[2], tmp_sensor_data.value[3], tmp_sensor_data.value[4]);

  /* Apply filtering */
  if(RGBCP_FILTER_ENABLE){
    static uint32_t filter_sum[NR_OF_SENSORS];
    int i;

    if(first_time){
      for(i=0;i<NR_OF_SENSORS;i++)
        filter_sum[i] = tmp_sensor_data.value[i]* RGBCP_FILTER_FACTOR;
      first_time=0;
      memcpy(&new_sensor_data, &tmp_sensor_data, sizeof(sensor_data));
    }
    else{
      for(i=0;i<NR_OF_SENSORS;i++){
        filter_sum[i] += tmp_sensor_data.value[i] - sensor_data.value[i];
        new_sensor_data.value[i] = filter_sum[i] / RGBCP_FILTER_FACTOR;
      }
    }
  }
  else{
    memcpy(&new_sensor_data, &tmp_sensor_data, sizeof(sensor_data));
  }

  /* Print filtered values */
  if(RGBCP_FILTER_ENABLE){
    TRACE("FILTERED: cdata: 0x%04X, rdata: 0x%04X, gdata: 0x%04X, bdata: 0x%04X, pdata: 0x%04X\n",
          new_sensor_data.value[0], new_sensor_data.value[1], new_sensor_data.value[2], new_sensor_data.value[3], new_sensor_data.value[4]);
  }

  /* Check if sensor values are changed */
  if (memcmp(&sensor_data, &new_sensor_data, sizeof(sensor_data)) != 0) {
    memcpy(&sensor_data, &new_sensor_data, sizeof(sensor_data));
    sensors_changed(&rgbc_sensor);
  }

  first_time=0;
  return true;
}
/*---------------------------------------------------------------------------*/
static void
tcs3772_init(void)
{
  uint8_t cmd, id;

  i2c_master_interface_init();

  cmd = REG_DEVICE_ID | COMMAND_BIT;
  if (i2c_master_read_reg(SLAVE_ADDRESS, &cmd, sizeof(cmd), &id, sizeof(id))) {
    TRACE("id: 0x%02X\n", id);
  }

  INFO("tcs3772 initialized");
}
/*---------------------------------------------------------------------------*/
static int
activate_sensor(void)
{
  uint8_t cmd[2];

  /* Set Integration time of RGBC sensor in steps of 2.4ms */
  cmd[0] = REG_RGBC_TIME | COMMAND_REPEAT | COMMAND_BIT;
  cmd[1] = RGBC_ATIME(RGBC_INTEGRATION_TIME);
  if (!i2c_master_write_reg(SLAVE_ADDRESS, cmd, sizeof(cmd))) {
    return false;
  }

  /* Set LED drive strenght and RGBC gain */
  cmd[0] = REG_CONTROL | COMMAND_REPEAT | COMMAND_BIT;
  cmd[1] = PDRIVE(PROX_LED_DRIVE_STRENGTH) | AGAIN(RGBC_GAIN);
  if (!i2c_master_write_reg(SLAVE_ADDRESS, cmd, sizeof(cmd))) {
    return false;
  }

  /* Set Nr. of proximity pulses generated */
  cmd[0] = REG_PROXIMITY_PULSE_CNT | COMMAND_REPEAT | COMMAND_BIT;
  cmd[1] = PROX_PULSES;
  if (!i2c_master_write_reg(SLAVE_ADDRESS, cmd, sizeof(cmd))) {
    return false;
  }

  /* Set Integration time of proximity sensor in steps of 2.4ms */
  cmd[0] = REG_PROXMITY_TIME | COMMAND_REPEAT | COMMAND_BIT;
  cmd[1] = PROXIMITY_TIME(PROX_INTEGRATION_TIME);
  if (!i2c_master_write_reg(SLAVE_ADDRESS, cmd, sizeof(cmd))) {
    return false;
  }

  /* Enable sensors */
  cmd[0] = REG_ENABLE | COMMAND_REPEAT | COMMAND_BIT;
  cmd[1] = ENABLE_PON | ENABLE_AEN | ENABLE_PEN;
  if (!i2c_master_write_reg(SLAVE_ADDRESS, cmd, sizeof(cmd))) {
    return false;
  }
  sensor_active = true;

  return true;
}
/*---------------------------------------------------------------------------*/
static int
deactivate_sensor(void)
{
  uint8_t cmd[2];

  cmd[0] = REG_ENABLE | COMMAND_REPEAT | COMMAND_BIT;
  cmd[1] = 0x00;
  if (!i2c_master_write_reg(SLAVE_ADDRESS, cmd, sizeof(cmd))) {
    return false;
  }

  sensor_active = false;

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  switch(type) {
  case TCS3772_RED:
  case TCS3772_GREEN:
  case TCS3772_BLUE:
  case TCS3772_CLEAR:
  case TCS3772_PROX:
    return sensor_data.value[type];
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
    tcs3772_init();
    etimer_set(&read_timer, TCS3772_DEFAULT_READ_INTERVAL);
    return 1;
  case SENSORS_ACTIVE:
    if(value) {
      activate_sensor();
      process_start(&tcs3772_process, 0);
    } else {
      deactivate_sensor();
      process_exit(&tcs3772_process);
    }
    return 1;
  case TCS3772_READ_INTERVAL:
    etimer_set(&read_timer, value);
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(tcs3772_process, ev, data)
{
  PROCESS_BEGIN();

  while(1) {
    etimer_restart(&read_timer);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&read_timer));

    update_values();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(rgbc_sensor, "RGBC Sensor", value, configure, status);
