#include <asf.h>

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "dev/leds.h"
#include "dev/watchdog.h"
#include "dev/serial-line.h"
#include "ip64.h"
#include "lib/random.h"
#include "net/netstack.h"
#include "net/queuebuf.h"
#include "net/ip/tcpip.h"
#include "net/ip/uip.h"
#include "net/mac/frame802154.h"
#include "simple-rpl.h"
#include "lib/sensors.h"
#include "log.h"
#include "dbg-arch.h"
#include "samr21-rf.h"


#define BOARD_STRING  BOARD_NAME


SENSORS(0);

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
static void
rpl_route_callback(int event, uip_ipaddr_t *route, uip_ipaddr_t *ipaddr,
                   int numroutes)
{
  static uint8_t has_rpl_route = 0;

  if(event == UIP_DS6_NOTIFICATION_DEFRT_ADD) {
    if (!has_rpl_route) {
      has_rpl_route = 1;
      INFO("Got first RPL route (num routes = %d)", numroutes);
    } else {
      INFO("Got RPL route (num routes = %d)", numroutes);
    }
  }
}
/*---------------------------------------------------------------------------*/
int
main(void)
{
  static struct uip_ds6_notification n;

  clock_init();

  /* Turn of CS of LCD */
  struct port_config pin_conf;
  port_get_config_defaults(&pin_conf);
  pin_conf.direction = PORT_PIN_DIR_OUTPUT;
  port_pin_set_config(DISPLAY_CS, &pin_conf);
  port_pin_set_output_level(DISPLAY_CS, true);

  leds_init();
  leds_off(LEDS_WHITE);
  leds_on(LEDS_GREEN);

  dbg_init();

  clock_wait(CLOCK_SECOND * 5);

  process_init();

//  watchdog_init();
//  watchdog_start();

  rtimer_init();

  INFO(CONTIKI_VERSION_STRING);
  INFO(BOARD_STRING);
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
  uip_ds6_notification_add(&n, rpl_route_callback);
  simple_rpl_init();
  ip64_init();
#endif /* UIP_CONF_IPV6 */

  energest_init();
  ENERGEST_ON(ENERGEST_TYPE_CPU);

  autostart_start(autostart_processes);

  process_start(&sensors_process, NULL);

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
  printf("%s\n", msg);
}
