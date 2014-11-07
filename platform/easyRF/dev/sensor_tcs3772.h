#ifndef SENSOR_TCS3772_H
#define SENSOR_TCS3772_H

#include "lib/sensors.h"

/* Configure */
#define TCS3772_READ_INTERVAL 1

/* Value */
#define TCS3772_RED          0
#define TCS3772_GREEN        1
#define TCS3772_BLUE         2
#define TCS3772_CLEAR        3
#define TCS3772_RED_BYTE     4
#define TCS3772_GREEN_BYTE   5
#define TCS3772_BLUE_BYTE    6
#define TCS3772_CLEAR_BYTE   7


extern const struct sensors_sensor rgbc_sensor;

#endif // SENSOR_TCS3772_H
