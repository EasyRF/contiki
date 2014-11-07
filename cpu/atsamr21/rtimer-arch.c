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

/* contiki */
#include "sys/energest.h"
#include "sys/rtimer.h"
#include "dev/leds.h"


//#define TEST_RTC_COUNTER


static struct rtc_module rtc_instance;


/*---------------------------------------------------------------------------*/
#ifdef TEST_RTC_COUNTER
void rtc_overflow_callback(void)
{
  ENERGEST_ON(ENERGEST_TYPE_IRQ);

  leds_toggle(LED1_GREEN);

  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}
#endif
/*---------------------------------------------------------------------------*/
void
rtc_compare_callback(void)
{
  ENERGEST_ON(ENERGEST_TYPE_IRQ);

  rtc_count_disable_callback(&rtc_instance, RTC_COUNT_CALLBACK_COMPARE_0);

  rtimer_run_next();

  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}
/*---------------------------------------------------------------------------*/
void
rtimer_arch_init(void)
{
  struct rtc_count_config config_rtc_count;

  /* Configure RTC in counter mode */
  rtc_count_get_config_defaults(&config_rtc_count);
  config_rtc_count.prescaler           = RTC_COUNT_PRESCALER_DIV_1;
  config_rtc_count.mode                = RTC_COUNT_MODE_32BIT;
  config_rtc_count.continuously_update = true;
  rtc_count_init(&rtc_instance, RTC, &config_rtc_count);

  /* Enable RTC */
  rtc_count_enable(&rtc_instance);

//  rtc_count_set_period(&rtc_instance, 0xFFFF);

#ifdef TEST_RTC_COUNTER
  /* Overflow callback and reload value */
  rtc_count_register_callback(
        &rtc_instance, rtc_overflow_callback, RTC_COUNT_CALLBACK_OVERFLOW);
  rtc_count_enable_callback(&rtc_instance, RTC_COUNT_CALLBACK_OVERFLOW);
  rtc_count_set_period(&rtc_instance, 2000);
#endif

  /* Compare callback */
  rtc_count_register_callback(
        &rtc_instance, rtc_compare_callback, RTC_COUNT_CALLBACK_COMPARE_0);
}
/*---------------------------------------------------------------------------*/
rtimer_clock_t
rtimer_arch_now(void)
{
  return rtc_count_get_count(&rtc_instance);
}
/*---------------------------------------------------------------------------*/
void
rtimer_arch_schedule(rtimer_clock_t t)
{
  rtimer_clock_t now;

  /* Retrieve current RTC counter value */
  now = RTIMER_NOW();

  /*
   * New value must be 5 ticks in the future. The ST may tick once while we're
   * writing the registers. We play it safe here and we add a bit of leeway
   */
  if ((rtimer_clock_t)(t - now) < 7) {
    t = now + 7;
  }

  /* Set value for compare (use channel 0) */
  rtc_count_set_compare(&rtc_instance, t, RTC_COUNT_COMPARE_0);

  /* Enable compare callback */
  rtc_count_enable_callback(&rtc_instance, RTC_COUNT_CALLBACK_COMPARE_0);
}
