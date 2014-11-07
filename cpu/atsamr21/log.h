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

#ifndef LOG_H_
#define LOG_H_

#include "contiki-conf.h"

/*
 * Logging using printf and log levels
 */
#if LOG_FORMATTED_CONF_ENABLED
#include <stdio.h>
#include "sys/clock.h"
extern const char* log_formatted_level_strings[];
#define LOG_FORMATTED(level, fmt, ...) do { printf("%s - %ld - ", log_formatted_level_strings[level], clock_time()); printf("%s: ", __func__); printf(fmt,  ##__VA_ARGS__ ); printf("\n"); } while(0)
#else
#define LOG_FORMATTED(level, fmt, ...)
#endif

/* Available log levels */
#define LOG_LEVEL_NONE  0
#define LOG_LEVEL_FATAL 1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_INFO  3
#define LOG_LEVEL_TRACE 4

/* Disable logging when no log level is specified */
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

#if (LOG_LEVEL >= LOG_LEVEL_TRACE)
  #define TRACE(fmt, ...)   LOG_FORMATTED(LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__ )
#else
  #define TRACE(fmt, ...)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_INFO)
  #define INFO(fmt, ...)    LOG_FORMATTED(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__ )
#else
  #define INFO(fmt, ...)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_WARN)
  #define WARN(fmt, ...)    LOG_FORMATTED(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__ )
#else
  #define WARN(fmt, ...)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_FATAL)
  #define FATAL(fmt, ...)   do { LOG_FORMATTED(LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__ ); while(1); } while(0)
#else
  #define FATAL(fmt, ...)   while (1);
#endif

#endif /* LOG_H_ */
