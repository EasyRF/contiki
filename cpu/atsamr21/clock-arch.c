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
#include <asf.h>
#include "contiki.h"
#include "sys/clock.h"
#include "sys/cc.h"
#include "sys/etimer.h"
#include "sys/energest.h"


static volatile clock_time_t current_clock = 0;
static volatile unsigned long current_seconds = 0;

#ifndef CONTIKI_NO_CORE
static clock_time_t second_countdown = CLOCK_SECOND;
#endif


/*---------------------------------------------------------------------------*/
#ifndef CONTIKI_NO_CORE
void
SysTick_Handler(void)
{
  ENERGEST_ON(ENERGEST_TYPE_IRQ);

  current_clock++;

  if(--second_countdown == 0) {
    current_seconds++;
    second_countdown = CLOCK_SECOND;
  }

  if(etimer_next_expiration_time() <= current_clock) {
    etimer_request_poll();
  }

  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}
#endif
/*---------------------------------------------------------------------------*/
void
clock_init(void)
{
  irq_initialize_vectors();
  cpu_irq_enable();
  sleepmgr_init();
  system_init();
  delay_init();
  SysTick_Config(system_cpu_clock_get_hz() / CLOCK_SECOND - 1UL);
}
/*---------------------------------------------------------------------------*/
clock_time_t
clock_time(void)
{
  return current_clock;
}
/*---------------------------------------------------------------------------*/
/**
 * Delay the CPU for dt microseconds
 */
void
clock_delay_usec(uint16_t dt)
{
  delay_us(dt);
}
/*---------------------------------------------------------------------------*/
/**
 * Wait for a multiple of 1 ms.
 */
void
clock_wait(clock_time_t i)
{
  clock_time_t start;

  start = clock_time();
  while(clock_time() - start < (clock_time_t) i);
}
/*---------------------------------------------------------------------------*/
unsigned long
clock_seconds(void)
{
  return current_seconds;
}
/*--------------------------------------------------------------------------*/
void
clock_set_seconds(unsigned long sec)
{
  current_seconds = sec;
}
/*--------------------------------------------------------------------------*/


