#ifndef SENSOR_BMP180_H
#define SENSOR_BMP180_H

#include "lib/sensors.h"

/* Configure */
#define BMP180_READ_INTERVAL  1

/* Values */
#define BMP180_TEMPERATURE    1
#define BMP180_PRESSURE       2

extern const struct sensors_sensor pressure_sensor;

#endif // SENSOR_BMP180_H
