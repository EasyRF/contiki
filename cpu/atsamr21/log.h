/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef LOG_H_
#define LOG_H_

#include "contiki-conf.h"

/*
 * Simple logging
 */
#if LOG_CONF_ENABLED
void log_message(const char *part1, const char *part2);
#else /* LOG_CONF_ENABLED */
#define log_message(p1, p2)
#endif /* LOG_CONF_ENABLED */


/*
 * Logging using printf and log levels
 */
#if LOG_FORMATTED_CONF_ENABLED
#include <stdio.h>
#include "sys/clock.h"
extern const char* log_formatted_level_strings[];
#define LOG_FORMATTED(level, fmt, ...) printf("%s - %ld - ", log_formatted_level_strings[level], clock_time()); printf("%s: ", __func__); printf(fmt,  ##__VA_ARGS__ ); printf("\n");
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
  #define FATAL(fmt, ...)   LOG_FORMATTED(LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__ ); while(1);
#else
  #define FATAL(fmt, ...)   while (1);
#endif

#endif /* LOG_H_ */
