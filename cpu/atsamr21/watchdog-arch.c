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
#include "dev/watchdog.h"
#include "log.h"

/*---------------------------------------------------------------------------*/
void
watchdog_init(void)
{
  /* Init is done in watchdog_start */
}
/*---------------------------------------------------------------------------*/
void
watchdog_start(void)
{
  struct wdt_conf config_wdt;
  wdt_get_config_defaults(&config_wdt);

  /* Set the Watchdog configuration settings
   * GCLK_GENERATOR_4 uses ULP (Ultra Low Power) clock which runs at 32 kHz
   * The prescaler is set to 32 so a clock speed of 1 kHz is obtained
   * Using WDT_PERIOD_4096CLK, the timeout is about 4 second
   */
  config_wdt.always_on      = false;
  config_wdt.clock_source   = GCLK_GENERATOR_4;
  config_wdt.timeout_period = WDT_PERIOD_4096CLK;

  /* Initialize and enable the Watchdog with the user settings */
  wdt_set_config(&config_wdt);

  if (system_get_reset_cause() == SYSTEM_RESET_CAUSE_WDT) {
    TRACE("last reset due to watchdog");
  }
}
/*---------------------------------------------------------------------------*/
void
watchdog_periodic(void)
{
  wdt_reset_count();
}
/*---------------------------------------------------------------------------*/
void
watchdog_stop(void)
{
  /* Once the watchdog is enabled it cannot be stopped */
  return;
}
/*---------------------------------------------------------------------------*/
void
watchdog_reboot(void)
{
  /* Keep control and let the watchdog kick in */
  while(1) {
  }
}
/*---------------------------------------------------------------------------*/
