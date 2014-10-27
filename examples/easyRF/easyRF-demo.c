#include "contiki.h"
#include "contiki-net.h"
#include "http-socket.h"
#include "log.h"

static uint8_t has_rpl_route = 0;

/*---------------------------------------------------------------------------*/
PROCESS(easyRF_demo_process, "EasyRF Demo Process");
AUTOSTART_PROCESSES(&easyRF_demo_process);
/*---------------------------------------------------------------------------*/
void http_socket_callback(struct http_socket *s,
                          void *ptr,
                          http_socket_event_t ev,
                          const uint8_t *data,
                          uint16_t datalen)
{
  TRACE("ev: %d, datalen: %d", ev, datalen);
}
/*---------------------------------------------------------------------------*/
static void
rpl_route_callback(int event, uip_ipaddr_t *route, uip_ipaddr_t *ipaddr,
                   int numroutes)
{
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
PROCESS_THREAD(easyRF_demo_process, ev, data)
{
  static struct uip_ds6_notification n;
  static struct etimer et;
  static struct http_socket hs;

  PROCESS_BEGIN();

  INFO("EasyRF Demo Process started");

  uip_ds6_notification_add(&n, rpl_route_callback);

  etimer_set(&et, CLOCK_SECOND * 10);

  while (1) {
    PROCESS_WAIT_UNTIL(etimer_expired(&et));

    if (has_rpl_route) {
      INFO("****** BEFORE GET ******");
      http_socket_get(&hs, "http://192.168.1.101:9000/api/things", http_socket_callback, 0);
    }

    etimer_restart(&et);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
