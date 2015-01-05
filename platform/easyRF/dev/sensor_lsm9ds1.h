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

#ifndef SENSOR_LSM9DS1_H
#define SENSOR_LSM9DS1_H

#include "conf_board.h"
#include "lib/sensors.h"

/* Configure */
#define LSM9DS1_READ_INTERVAL 1

/* Value */
#define LSM9DS1_TEMP          0
#define LSM9DS1_GYRO_X        1
#define LSM9DS1_GYRO_Y        2
#define LSM9DS1_GYRO_Z        3
#define LSM9DS1_ACC_X         4
#define LSM9DS1_ACC_Y         5
#define LSM9DS1_ACC_Z         6
#define LSM9DS1_COMPASS_X     7
#define LSM9DS1_COMPASS_Y     8
#define LSM9DS1_COMPASS_Z     9

extern const struct sensors_sensor nineaxis_sensor;

#endif // SENSOR_TCS3772_H
