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
