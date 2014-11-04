#include <asf.h>

#include "contiki.h"
#include "dev/leds.h"
#include "dev/watchdog.h"
#include "dev/button-sensor.h"
#include "dev/serial-line.h"
#include "dev/slip.h"
#include "lib/random.h"
#include "net/netstack.h"
#include "net/queuebuf.h"
#include "net/ip/tcpip.h"
#include "net/ip/uip.h"
#include "net/mac/frame802154.h"
#include "log.h"
#include "dbg-arch.h"
#include "samr21-rf.h"
#include "dev/button-sensor.h"

#include <stdio.h>


SENSORS(&button_sensor);


/*---------------------------------------------------------------------------*/
static void
set_rf_params(void)
{
  uint16_t short_addr;
  uint8_t ext_addr[8];

  uint32_t hw_mac_address_lower = (*((uint32_t *)0x0080A00C) ^ *((uint32_t *)0x0080A040));
  uint32_t hw_mac_address_upper = (*((uint32_t *)0x0080A044) ^ *((uint32_t *)0x0080A048));

  memcpy(&ext_addr[0], &hw_mac_address_lower, 4);
  memcpy(&ext_addr[4], &hw_mac_address_upper, 4);

  short_addr = ext_addr[7];
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
int
main(void)
{
  clock_init();

  leds_init();
  leds_on(LEDS_GREEN);

  dbg_init();

  INFO("Main CPU clock: %ld", system_cpu_clock_get_hz());

  process_init();

  watchdog_init();
  watchdog_start();

  rtimer_init();

  INFO(CONTIKI_VERSION_STRING);
  INFO(BOARD_NAME);
  INFO(" Net: %s", NETSTACK_NETWORK.name);
  INFO(" MAC: %s", NETSTACK_MAC.name);
  INFO(" RDC: %s", NETSTACK_RDC.name);

  process_start(&etimer_process, NULL);
  ctimer_init();

  netstack_init();
  set_rf_params();

  /* Use RF transceiver RNG, so call random_init after netstack_init */
  random_init(0);

#if UIP_CONF_IPV6
  memcpy(&uip_lladdr.addr, &linkaddr_node_addr, sizeof(uip_lladdr.addr));
  queuebuf_init();
  process_start(&tcpip_process, NULL);
#endif /* UIP_CONF_IPV6 */

  energest_init();
  ENERGEST_ON(ENERGEST_TYPE_CPU);

  process_start(&sensors_process, NULL);

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
