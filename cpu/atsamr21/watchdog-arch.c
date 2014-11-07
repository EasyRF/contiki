
/*
 * Copyright (c) 2010, Mariano Alvira <mar@devl.org> and other contributors
 * to the MC1322x project (http://mc1322x.devl.org) and Contiki.
 *
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
 * This file is part of the Contiki OS.
 *
 *
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
