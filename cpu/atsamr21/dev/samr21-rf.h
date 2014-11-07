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
#ifndef AT86RF233_DRIVER_H
#define AT86RF233_DRIVER_H

#include "contiki.h"
/*---------------------------------------------------------------------------
 * RF Config
 *---------------------------------------------------------------------------*/
/* Constants */
//#define SAMR21_RF_CCA_THRES_USER_GUIDE 0xF8
//#define SAMR21_RF_TX_POWER_RECOMMENDED 0xD5 /* ToDo: Determine value */
#define SAMR21_RF_CHANNEL_MIN            11
#define SAMR21_RF_CHANNEL_MAX            26
#define SAMR21_RF_CHANNEL_SPACING         5
#define SAMR21_RF_CHANNEL_SET_ERROR      -1
#define SAMR21_RF_MAX_PACKET_LEN        127
#define SAMR21_RF_MIN_PACKET_LEN          4
#define SAMR21_RF_CCA_CLEAR               1
#define SAMR21_RF_CCA_BUSY                0
#define SAMR21_CCA_CHANNEL_MASK           0x1f
/*---------------------------------------------------------------------------*/
#ifdef SAMR21_RF_CONF_TX_POWER
#define SAMR21_RF_TX_POWER SAMR21_RF_CONF_TX_POWER
#else
#define SAMR21_RF_TX_POWER SAMR21_RF_TX_POWER_RECOMMENDED
#endif /* SAMR21_RF_CONF_TX_POWER */

#ifdef SAMR21_RF_CONF_CCA_THRES
#define SAMR21_RF_CCA_THRES SAMR21_RF_CONF_CCA_THRES
#else
#define SAMR21_RF_CCA_THRES CCA_THRES_USER_GUIDE /** User guide recommendation */
#endif /* SAMR21_RF_CONF_CCA_THRES */

#ifdef SAMR21_RF_CONF_CHANNEL
#define SAMR21_RF_CHANNEL SAMR21_RF_CONF_CHANNEL
#else
#define SAMR21_RF_CHANNEL 11
#endif /* SAMR21_RF_CONF_CHANNEL */

#ifdef SAMR21_RF_CONF_AUTOACK
#define SAMR21_RF_AUTOACK SAMR21_RF_CONF_AUTOACK
#else
#define SAMR21_RF_AUTOACK 1
#endif /* SAMR21_RF_CONF_AUTOACK */
/*---------------------------------------------------------------------------*/
/** The NETSTACK data structure for the SAMR21 RF driver */
extern const struct radio_driver samr21_rf_driver;
/*---------------------------------------------------------------------------*/
unsigned short samr21_random_rand(void);
/*---------------------------------------------------------------------------*/
#endif // AT86RF233_DRIVER_H
