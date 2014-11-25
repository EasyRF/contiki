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
