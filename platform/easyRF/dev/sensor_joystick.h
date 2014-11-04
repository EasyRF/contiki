#ifndef SENSOR_JOYSTICK_H
#define NAVIGATION_JOYSTICK_H

#include "lib/sensors.h"

#define JOYSTICK_IDLE         0
#define JOYSTICK_UP           1
#define JOYSTICK_RIGHT        2
#define JOYSTICK_DOWN         3
#define JOYSTICK_LEFT         4
#define JOYSTICK_BUTTON       5

extern const struct sensors_sensor joystick_sensor;

#endif // SENSOR_JOYSTICK_H
