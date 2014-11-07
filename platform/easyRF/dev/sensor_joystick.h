#ifndef SENSOR_JOYSTICK_H
#define NAVIGATION_JOYSTICK_H

#include "lib/sensors.h"

/* Configure */
#define JOYSTICK_READ_INTERVAL  1

/* Value */
#define JOYSTICK_STATE        1

/* States */
#define JOYSTICK_IDLE         0
#define JOYSTICK_UP           1
#define JOYSTICK_RIGHT        2
#define JOYSTICK_DOWN         3
#define JOYSTICK_LEFT         4
#define JOYSTICK_BUTTON       5

/* State to string */
#define JOYSTICK_STATE_TO_STRING(s) (joystick_state_strings[s])

extern const char * joystick_state_strings[];
extern const struct sensors_sensor joystick_sensor;

#endif // SENSOR_JOYSTICK_H
