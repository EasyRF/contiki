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
#include "sensor_lsm9ds1.h"
#include "log.h"

#undef TRACE
#define TRACE(...)

/*** Sensor settings ***/

/* Gyroscope + accelerometer measurement rate (0=OFF, 1=14.9 Hz, 2=59.5 Hz, 3=119 Hz, 4=238 Hz, 5=476 Hz, 6=952 Hz) */
#ifndef ACCGYR_RATE
#define ACCGYR_RATE               1
#endif
/* Gyroscope bandwidth selection. Lower means slower and less noise (0..3) */
#ifndef GYR_BANDWIDTH
#define GYR_BANDWIDTH             0
#endif
/* Gyroscope full scale (0=245dps, 1=500dps, 3=2000dps) */
#ifndef GYR_FULLSCALE
#define GYR_FULLSCALE             0
#endif
/* Magnetic sensor mode (0=continuous, 1=single, 2.3=power down) */
#ifndef MAGN_MODE
#define MAGN_MODE                 0
#endif
/* Magnetic sensor: temperature compensation (0=OFF, 1=ON) */
#ifndef MAGN_TEMPCOMP
#define MAGN_TEMPCOMP             1
#endif
/* Magnetic sensor: XY operation mode (0=Low-power, 1=medium, 2=high, 3=ultra-high performance) */
#ifndef MAGN_XY_MODE
#define MAGN_XY_MODE              3
#endif
/* Magnetic sensor: Data rate (0=0.625, 1=1.25, 2=2.5, 3=5 ... 7=80 Hz) */
#ifndef MAGN_RATE
#define MAGN_RATE                 0
#endif


/*** Handy macros ***/

/* Bitshift helper */
#define BM(pos)                 ((uint32_t)1 << pos)

/* Auto increment bit helper */
#define COMMAND_AUTO_INC        BM(7)

/* Macro to get register id from struct */
#define REG_ADDR(regstruct,reg)  (    (uint8_t) ( (uint8_t *)&regstruct.reg - (uint8_t *)&regstruct - 1 )    )

#define GYROACC_READ_ALL    read_all_register(ACCGYR_SLAVE_ADDRESS, ((uint8_t *)&accgyr_reg) + 1, sizeof(struct accgyr_regs)-1)
#define GYROACC_WRITE_ALL   write_all_register(ACCGYR_SLAVE_ADDRESS,((uint8_t *)&accgyr_reg) + 0, sizeof(struct accgyr_regs)-0)

#define MAGN_READ_ALL       read_all_register(MAGN_SLAVE_ADDRESS,   ((uint8_t *)&magn_reg  ) + 1, sizeof(struct magn_regs  )-1)
#define MAGN_WRITE_ALL      write_all_register(MAGN_SLAVE_ADDRESS,  ((uint8_t *)&magn_reg  ) + 0, sizeof(struct magn_regs  )-0)

/* Macros to read specific registers */
#define GYROACC_READ(reg,length)   do{ uint8_t i=REG_ADDR(accgyr_reg,reg); i2c_master_read_reg(ACCGYR_SLAVE_ADDRESS, &i, sizeof(i), ((uint8_t *)&accgyr_reg.reg), length); }while(0)
#define MAGN_READ(reg,length)      do{ uint8_t i=REG_ADDR(magn_reg,reg);   i2c_master_read_reg(MAGN_SLAVE_ADDRESS,   &i, sizeof(i), ((uint8_t *)&magn_reg.reg),   length); }while(0)


/*** Accelerometer AND gyroscope settings ***/

/* Device address */
#define ACCGYR_SLAVE_ADDRESS    0x6A

struct accgyr_regs {
  uint8_t startaddr;          /* X: Set to 0 in init */
  uint8_t reserved1[4];       /* X: */
  uint8_t ACT_THS;            /* RW: */
  uint8_t ACT_DUR;            /* RW: */
  uint8_t INT_GEN_CFG_XL;     /* RW: */
  uint8_t INT_GEN_THS_XL[3];  /* RW: X,Y,Z */
  uint8_t INT_GEN_DUR_XL;     /* RW: */
  uint8_t REFERENCE_G;        /* RW: */
  uint8_t INT1_CTRL;          /* RW: */
  uint8_t INT2_CTRL;          /* RW: */
  uint8_t reserved2;          /* X: */
  uint8_t WHO_AM_I;           /* R: */
  uint8_t CTRL_REG1_G;        /* RW: */
  uint8_t CTRL_REG2_G;        /* RW: */
  uint8_t CTRL_REG3_G;        /* RW: */
  uint8_t ORIENT_CFG_G;       /* RW: */
  uint8_t INT_GEN_SRC_G;      /* R: */
  int16_t OUT_TEMP;           /* R: */
  uint8_t STATUS_REG;         /* R: */
  int16_t OUT_G[3];           /* R: X,Y,Z */
  uint8_t CTRL_REG4;          /* RW: */
  uint8_t CTRL_REG5_XL;       /* RW: */
  uint8_t CTRL_REG6_XL;       /* RW: */
  uint8_t CTRL_REG7_XL;       /* RW: */
  uint8_t CTRL_REG8;          /* RW: */
  uint8_t CTRL_REG9;          /* RW: */
  uint8_t CTRL_REG10;         /* RW: */
  uint8_t Reserved3;          /* X: */
  uint8_t INT_GEN_SRC_XL;     /* R: */
  uint8_t ACCGYR_STATUS_REG;  /* R: */
  int16_t OUT_XL[3];          /* R: X,Y,Z */
  uint8_t FIFO_CTRL;          /* RW: */
  uint8_t FIFO_SRC;           /* R: */
  uint8_t INT_GEN_CFG_G;      /* RW: */
  uint16_t INT_GEN_THS_G[3];  /* RW: X,Y,Z MSB first */
  uint8_t INT_GEN_DUR_G;      /* RW: */
} __attribute__((packed));

static struct accgyr_regs accgyr_reg;

/*** Magnetic sensor ***/

struct magn_regs {
  uint8_t startaddr;          /* X: Set to 0 in init */
  uint8_t reserved1[5];       /* X: */
  uint16_t OFFSET_REG_M[3];   /* RW: X,Y,Z */
  uint8_t reserved2[4];       /* X: */
  uint8_t WHO_AM_I;           /* R: */
  uint8_t reserved3[16];      /* X: */
  uint8_t CTRL_REG1_M;        /* RW: */
  uint8_t CTRL_REG2_M;        /* RW: */
  uint8_t CTRL_REG3_M;        /* RW: */
  uint8_t CTRL_REG4_M;        /* RW: */
  uint8_t CTRL_REG5_M;        /* RW: */
  uint8_t reserved4[2];       /* X: */
  uint8_t STATUS_REG;         /* R: */
  uint16_t OUT_M[3];          /* R: X,Y,Z */
  uint8_t reserved5[2];       /* X: */
  uint8_t INT_CFG_M;          /* RW: */
  uint8_t INT_SRC_M;          /* RW: */
  uint16_t INT_THS_M;         /* RW: */
} __attribute__((packed));

static struct magn_regs magn_reg;

/* Device address */
#define MAGN_SLAVE_ADDRESS       0x1C


/***********************/

/* Number of times to retry I2C read/writes */
#define TIMEOUT                 10

/* Update interval */
#define LSM9DS1_DEFAULT_READ_INTERVAL   (CLOCK_SECOND)

struct status_regs {
  uint8_t   status;
  int16_t   temperature;
  int16_t   gyro[3];
  int16_t   acceleration[3];
  int16_t   magnetic[3];
} __attribute__ ((packed));


static bool sensor_active;
static struct status_regs sensor_data;
static struct etimer read_timer;

/*---------------------------------------------------------------------------*/
PROCESS(lsm9ds1_process, "LSM9DS1 Process");
/*---------------------------------------------------------------------------*/
static bool
read_all_register(uint8_t slaveaddr, uint8_t * regs, uint8_t sizeofstruct)
{
  /*Read all registers in once is not possible due to memory hops to enable BURST read (Chapter 3.3 of datasheet) */
  uint8_t i=0;
  for(i=0;i<sizeofstruct;i++){
    if (!i2c_master_read_reg(slaveaddr, &i, sizeof(i), regs+i, 1)) {
      return false;
    }
  }

  //  for(i=0;i<sizeofstruct;i++){
  //    TRACE("reg: 0x%02X:0x%02X", i,regs[i]);
  //  }
  return true;
}

/*---------------------------------------------------------------------------*/
static bool
write_all_register(uint8_t slaveaddr, uint8_t * regs, uint8_t sizeofstruct)
{
  uint8_t i=0;
  if (!i2c_master_write_reg(slaveaddr, regs, sizeofstruct)) {
    return false;
  }

  //  for(i=0;i<sizeofstruct;i++){
  //    TRACE("reg: 0x%02X:0x%02X", i,regs[i]);
  //  }
  return true;
}
/*---------------------------------------------------------------------------*/
static bool
update_values(void)
{
  struct status_regs tmp_sensor_data;

  GYROACC_READ(OUT_TEMP,2);
  GYROACC_READ(OUT_G[0],6);
  GYROACC_READ(OUT_XL[0],6);
  MAGN_READ(OUT_M[0],6);

  memcpy(tmp_sensor_data.gyro,          accgyr_reg.OUT_G,   sizeof(tmp_sensor_data.gyro));
  memcpy(tmp_sensor_data.acceleration,  accgyr_reg.OUT_XL,  sizeof(tmp_sensor_data.acceleration));
  memcpy(tmp_sensor_data.magnetic,      magn_reg.OUT_M,     sizeof(tmp_sensor_data.gyro));
  tmp_sensor_data.temperature           = (int32_t) accgyr_reg.OUT_TEMP * 10 / 16 + 250; /* temperature sensor result is 16 LSB/degree, 0 is 25 degrees */

  TRACE("status TEMP %d GYRO %6d.%6d.%6d ACC %6d.%6d.%6d MAGN %6d.%6d.%6d",
        tmp_sensor_data.temperature, tmp_sensor_data.gyro[0], tmp_sensor_data.gyro[1], tmp_sensor_data.gyro[2], tmp_sensor_data.acceleration[0], tmp_sensor_data.acceleration[1], tmp_sensor_data.acceleration[2], tmp_sensor_data.magnetic[0], tmp_sensor_data.magnetic[1], tmp_sensor_data.magnetic[2]);

  /* Check if sensor values are changed */
  if (memcmp(&sensor_data, &tmp_sensor_data, sizeof(sensor_data)) != 0) {
    memcpy(&sensor_data, &tmp_sensor_data, sizeof(sensor_data));
    sensors_changed(&nineaxis_sensor);
  }

  return true;
}
/*---------------------------------------------------------------------------*/
static void
lsm9ds1_init(void)
{
  uint8_t regaddr, id;

  i2c_master_interface_init();

  accgyr_reg.startaddr=0;
  if(GYROACC_READ_ALL){
    TRACE("gyro+acc id: 0x%02X\n", accgyr_reg.WHO_AM_I);
  }

  magn_reg.startaddr=0;
  if(MAGN_READ_ALL){
    TRACE("magn id: 0x%02X\n", magn_reg.WHO_AM_I);
  }

  INFO("lsm9ds1 initialized");
}
/*---------------------------------------------------------------------------*/
static int
activate_sensor(void)
{
  /* Activate Gyroscope + accelerometer */
  accgyr_reg.CTRL_REG1_G = (ACCGYR_RATE<<5) | (GYR_FULLSCALE<<3) | (GYR_BANDWIDTH<<0);
  /* Critical settings, Block data update enabled, Auto Increment enabled */
  accgyr_reg.CTRL_REG8 = (1<<6) | (1<<2);
  /* Activate magnetic sensor */
  magn_reg.CTRL_REG3_M = MAGN_MODE;
  /* Activate temperature compensation, set datarate and XY mode */
  magn_reg.CTRL_REG1_M = (MAGN_TEMPCOMP<<7) | (MAGN_XY_MODE<<5) | (MAGN_RATE<<2);

  GYROACC_WRITE_ALL;
  MAGN_WRITE_ALL;

  sensor_active = true;
  return true;
}
/*---------------------------------------------------------------------------*/
static int
deactivate_sensor(void)
{
  sensor_active = false;

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  switch(type) {
  case LSM9DS1_TEMP:
    return sensor_data.temperature;
  case LSM9DS1_GYRO_X:
  case LSM9DS1_GYRO_Y:
  case LSM9DS1_GYRO_Z:
    return sensor_data.gyro[type-LSM9DS1_GYRO_X];
  case LSM9DS1_ACC_X:
  case LSM9DS1_ACC_Y:
  case LSM9DS1_ACC_Z:
    return sensor_data.acceleration[type-LSM9DS1_ACC_X];
  case LSM9DS1_COMPASS_X:
  case LSM9DS1_COMPASS_Y:
  case LSM9DS1_COMPASS_Z:
    return sensor_data.magnetic[type-LSM9DS1_COMPASS_X];
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
    lsm9ds1_init();
    etimer_set(&read_timer, LSM9DS1_DEFAULT_READ_INTERVAL);
    return 1;
  case SENSORS_ACTIVE:
    if(value) {
      activate_sensor();
      process_start(&lsm9ds1_process, 0);
    } else {
      deactivate_sensor();
      process_exit(&lsm9ds1_process);
    }
    return 1;
  case LSM9DS1_READ_INTERVAL:
    etimer_set(&read_timer, value);
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(lsm9ds1_process, ev, data)
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
SENSORS_SENSOR(nineaxis_sensor, "NINE-AXIS Sensor", value, configure, status);
