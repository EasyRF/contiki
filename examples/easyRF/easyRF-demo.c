#include "contiki.h"
#include "contiki-net.h"
#include "http-socket.h"
#include "log.h"

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
PROCESS_THREAD(easyRF_demo_process, ev, data)
{
  static struct etimer et;
  static struct http_socket hs;

  PROCESS_BEGIN();

  etimer_set(&et, CLOCK_SECOND * 10);
  PROCESS_WAIT_UNTIL(etimer_expired(&et));

  INFO("EasyRF Demo Process started");

  etimer_restart(&et);

  while (1) {
    PROCESS_YIELD();
    if(etimer_expired(&et)) {
      http_socket_get(&hs, "http://192.168.1.101:9000/api/things", http_socket_callback, 0);
      etimer_restart(&et);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
