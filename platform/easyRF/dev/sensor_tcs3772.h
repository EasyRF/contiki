#ifndef SENSOR_TCS3772_H
#define SENSOR_TCS3772_H

#include "lib/sensors.h"

#define RGBC_RED     0
#define RGBC_GREEN   1
#define RGBC_BLUE    2
#define RGBC_CLEAR   3

#define RGBC_RED_BYTE     4
#define RGBC_GREEN_BYTE   5
#define RGBC_BLUE_BYTE    6
#define RGBC_CLEAR_BYTE   7


extern const struct sensors_sensor rgbc_sensor;

#endif // SENSOR_TCS3772_H
