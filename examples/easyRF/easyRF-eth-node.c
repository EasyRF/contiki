#include "contiki.h"
#include "contiki-net.h"
#include "http-socket.h"
#include "log.h"

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"


#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])


static struct uip_udp_conn *server_conn;
static uint8_t sensor_data[128];
static uint8_t sensor_data_valid;

/*---------------------------------------------------------------------------*/
PROCESS(http_post_process, "HTTP POST Process");
PROCESS(udp_server_process, "UDP server process");
AUTOSTART_PROCESSES(&resolv_process, &udp_server_process, &http_post_process);
/*---------------------------------------------------------------------------*/
void http_socket_callback(struct http_socket *s,
                          void *ptr,
                          http_socket_event_t ev,
                          const uint8_t *data,
                          uint16_t datalen)
{
//  TRACE("ev: %d, datalen: %d", ev, datalen);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(http_post_process, ev, data)
{
  static struct etimer et;
  static struct http_socket hs;

  PROCESS_BEGIN();

  INFO("EasyRF Ethernet Node Process started");

  etimer_set(&et, CLOCK_SECOND * 1);

  while (1) {
    PROCESS_WAIT_UNTIL(etimer_expired(&et));

    if (sensor_data_valid) {
      http_socket_post(&hs, "http://192.168.1.36:9000/api/devices/1",
                       sensor_data, strlen((const char *)sensor_data),
                       "application/json", http_socket_callback, 0);
    } else {
      INFO("waiting for sensor data");
    }

    etimer_restart(&et);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
  if(uip_newdata()) {
    ((char *)uip_appdata)[uip_datalen()] = 0;
    PRINTF("Server received: '%s' from ", (char *)uip_appdata);
    PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
    PRINTF("\n");

    memcpy(sensor_data, uip_appdata, uip_datalen());

    sensor_data_valid = 1;
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
#if UIP_CONF_ROUTER
  uip_ipaddr_t ipaddr;
#endif /* UIP_CONF_ROUTER */

  PROCESS_BEGIN();

  PRINTF("UDP server started\n");

#if RESOLV_CONF_SUPPORTS_MDNS
  resolv_set_hostname("contiki-udp-server");
#endif

#if UIP_CONF_ROUTER
  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
#endif /* UIP_CONF_ROUTER */

  server_conn = udp_new(NULL, UIP_HTONS(3001), NULL);
  udp_bind(server_conn, UIP_HTONS(3000));

  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      tcpip_handler();
    }
  }

  PROCESS_END();
}
