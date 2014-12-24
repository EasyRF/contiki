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
#ifndef CONF_BOARD_H
#define CONF_BOARD_H

/* MOST CPU Peripheral settings are defined in user_board.h */

/*** RF chip ***/

/* Trim capacitance for Crystal tuning */
#define RF_CAP_TRIM                      0x0F

/* SPI speed to RF chip */
#define AT86RFX_SPI_BAUDRATE             5000000UL


/*** Light/Color/Proximity Sensor settings ***/

/* Filter sensor values */
#define RGBCP_FILTER_ENABLE      1
/* High value means less noise and slower detection */
#define RGBCP_FILTER_FACTOR      5
/* Nr. of proximity pulses generated (0..255) */
#define PROX_PULSES              20
/* Integration time of proximity sensor in steps of 2.4ms (1..256) */
#define PROX_INTEGRATION_TIME    5
/* Proximity LED Drive Strength (0=100mA, 1=50mA, 2=25mA, 3=12.5mA) */
#define PROX_LED_DRIVE_STRENGTH  0
/* RGBC Gain Control (0=1x, 1=4x, 2=16x, 3=64x) */
#define RGBC_GAIN                2
/* Integration time of RGBC sensor in steps of 2.4ms (1..256) */
#define RGBC_INTEGRATION_TIME    5
/* Approximated cylce time */
#define TCS3772_CYCLE_MS         ((uint16_t)((RGBC_INTEGRATION_TIME+PROX_INTEGRATION_TIME+1)*2.4))



#endif // CONF_BOARD_H
