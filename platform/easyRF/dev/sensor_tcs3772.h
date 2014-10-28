#ifndef SENSOR_TCS3772_H
#define SENSOR_TCS3772_H

#include "lib/sensors.h"

#define RED     0
#define GREEN   1
#define BLUE    2
#define CLEAR   3

#define RED_BYTE     4
#define GREEN_BYTE   5
#define BLUE_BYTE    6
#define CLEAR_BYTE   7


extern const struct sensors_sensor rgbc_sensor;

#endif // SENSOR_TCS3772_H
