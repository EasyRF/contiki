#ifndef SENSOR_SI7020_H
#define SENSOR_SI7020_H

#include "lib/sensors.h"

/* Configure */
#define SI7020_READ_INTERVAL  1

/* Value */
#define SI7020_HUMIDITY     1
#define SI7020_TEMPERATURE  2

extern const struct sensors_sensor rh_sensor;

#endif // SENSOR_SI7020_H
