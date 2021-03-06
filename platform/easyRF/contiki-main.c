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
#include "contiki-lib.h"
#include "contiki-net.h"
#include "dev/leds.h"
#include "dev/watchdog.h"
#include "dev/serial-line.h"
#include "dev/sensor_tcs3772.h"
#include "dev/sensor_bmp180.h"
#include "dev/sensor_si7020.h"
#include "dev/sensor_joystick.h"
#include "dev/sensor_qtouch_wheel.h"
#include "dev/sensor_lsm9ds1.h"
#include "dev/display_st7565s.h"
#include "flash.h"
#include "cfs-coffee.h"
#include "ip64.h"
#include "lib/random.h"
#include "net/netstack.h"
#include "net/queuebuf.h"
#include "net/ip/tcpip.h"
#include "net/ip/uip.h"
#include "net/mac/frame802154.h"
#include "lib/sensors.h"
#include "simple-rpl.h"
#include "log.h"
#include "dbg-arch.h"
#include "samr21-rf.h"
#include "stack_test.h"


SENSORS(&pressure_sensor, &rgbc_sensor, &rh_sensor, &touch_wheel_sensor, &joystick_sensor, &nineaxis_sensor);


/*---------------------------------------------------------------------------*/
static void
set_rf_params(void)
{
  uint16_t short_addr;
  uint8_t ext_addr[8];

  uint32_t hw_serial_lower = (*((uint32_t *)0x0080A00C) ^ *((uint32_t *)0x0080A040));
  uint32_t hw_serial_upper = (*((uint32_t *)0x0080A044) ^ *((uint32_t *)0x0080A048));

  memcpy(&ext_addr[0], &hw_serial_lower, 4);
  memcpy(&ext_addr[4], &hw_serial_upper, 4);

  short_addr  = ext_addr[7];
  short_addr |= ext_addr[6] << 8;

  /* Populate linkaddr_node_addr. Maintain endianness */
  memcpy(&linkaddr_node_addr, &ext_addr[8 - LINKADDR_SIZE], LINKADDR_SIZE);

#if STARTUP_CONF_VERBOSE
  {
    int i;
    printf("Rime configured with address ");
    for(i = 0; i < LINKADDR_SIZE - 1; i++) {
      printf("%02x:", linkaddr_node_addr.u8[i]);
    }
    printf("%02x\n", linkaddr_node_addr.u8[i]);
  }
#endif

  NETSTACK_RADIO.set_value(RADIO_PARAM_PAN_ID, IEEE802154_PANID);
  NETSTACK_RADIO.set_value(RADIO_PARAM_16BIT_ADDR, short_addr);
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, SAMR21_RF_CHANNEL);
  NETSTACK_RADIO.set_object(RADIO_PARAM_64BIT_ADDR, ext_addr, 8);
}
/*---------------------------------------------------------------------------*/
void dhcp_callback(uint8_t configured)
{
  if (configured) {
    /* Set us up as a RPL root node. */
    simple_rpl_init_dag_immediately();
  }
}
/*---------------------------------------------------------------------------*/
int
main(void)
{
  write_aa_to_stack();

  clock_init();

  leds_init();
  leds_off(LEDS_ALL);
  display_st7565s.init();

  dbg_init();
#if DBG_CONF_USB == 1
  clock_wait(CLOCK_SECOND * 1);
#endif

  INFO("Main CPU clock: %ld", system_cpu_clock_get_hz());

  print_stack_info();

  process_init();

  watchdog_init();
  watchdog_start();
  watchdog_periodic();

  rtimer_init();

  INFO(CONTIKI_VERSION_STRING);
  INFO(BOARD_NAME);
  INFO("Net: %s", NETSTACK_NETWORK.name);
  INFO("MAC: %s", NETSTACK_MAC.name);
  INFO("RDC: %s", NETSTACK_RDC.name);

  process_start(&etimer_process, NULL);
  ctimer_init();

  netstack_init();
  set_rf_params();

  /* Use RF transceiver RNG, so call random_init after netstack_init */
  random_init(0);

  watchdog_periodic();

#if UIP_CONF_IPV6
  memcpy(&uip_lladdr.addr, &linkaddr_node_addr, sizeof(uip_lladdr.addr));
  queuebuf_init();
  process_start(&tcpip_process, NULL);
  simple_rpl_init();
  if (ip64_init() == 0) {
    INFO("Ethernet interface found");
  } else {
    INFO("NO ethernet interface found");
  }
#endif /* UIP_CONF_IPV6 */

  energest_init();
  ENERGEST_ON(ENERGEST_TYPE_CPU);

  watchdog_periodic();

  autostart_start(autostart_processes);

  while(1) {
    uint8_t r;
    do {
      /* Reset watchdog and handle polls and events */
      watchdog_periodic();

      r = process_run();
    } while(r > 0);
  }
}
/*---------------------------------------------------------------------------*/
void uip_log(char *msg)
{
  INFO(msg);
}
