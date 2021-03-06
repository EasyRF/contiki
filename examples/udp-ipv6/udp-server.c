/*
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
 * This file is part of the Contiki operating system.
 *
 */

#include "compiler.h"
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "dev/leds.h"
#include "log.h"
#include <string.h>

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define MAX_PAYLOAD_LEN 120

static struct uip_udp_conn *server_conn;

#define MAX_CLIENTS 10
struct client_data {
  uip_ipaddr_t src_addr;
  uint8_t last_seq_id;
  bool used;
};
static struct client_data client_data[MAX_CLIENTS];

PROCESS(udp_server_process, "UDP server process");
AUTOSTART_PROCESSES(&resolv_process,&udp_server_process);
/*---------------------------------------------------------------------------*/
static struct client_data *
get_or_create_client_data(uip_ipaddr_t * src_addr)
{
  int i;

  for (i = 0; i < MAX_CLIENTS; i++) {
    struct client_data * data = &client_data[i];
    if (data->used && uip_ipaddr_cmp(&data->src_addr, src_addr)) {
      return data;
    }
  }

  for (i = 0; i < MAX_CLIENTS; i++) {
    struct client_data * data = &client_data[i];
    if (data->used == false) {
      uip_ipaddr_copy(&data->src_addr, src_addr);
      data->last_seq_id = 0;
      data->used = true;
      return data;
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
//  static clock_time_t previous = 0;
//  static int last_seq_id = 0;
//  char buf[MAX_PAYLOAD_LEN];

  if(uip_newdata()) {

    struct client_data * data = get_or_create_client_data(&UIP_IP_BUF->srcipaddr);
    if (!data) {
      WARN("No more room for clients");
      return;
    }

    uint8_t current_seq_id = ((uint8_t *)uip_appdata)[0];

    if ((uint8_t)(data->last_seq_id + 1) == current_seq_id) {
      leds_toggle(LEDS_GREEN);
      leds_off(LEDS_RED);
    } else {
      leds_off(LEDS_GREEN);
      leds_on(LEDS_RED);
      WARN("Invalid seq. Expected %d got %d", (uint8_t)(data->last_seq_id + 1), current_seq_id);
    }

    data->last_seq_id = current_seq_id;

//    clock_time_t now = clock_time();

//    printf("delta: %ld\n", now - previous);

//    leds_toggle(LEDS_GREEN);

//    previous = now;
//    ((char *)uip_appdata)[uip_datalen()] = 0;
//    INFO("Server received: '%s' from ", (char *)uip_appdata);
//    PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
//    PRINTF("\n");

//    uip_ipaddr_copy(&server_conn->ripaddr, &UIP_IP_BUF->srcipaddr);
//    PRINTF("Responding with message: ");
//    sprintf(buf, "Hello from the server! (%d)", ++seq_id);
//    PRINTF("%s\n", buf);

//    uip_udp_packet_send(server_conn, buf, strlen(buf));
    /* Restore server connection to allow data from any node */
//    memset(&server_conn->ripaddr, 0, sizeof(server_conn->ripaddr));
  }
}
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Server IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
    }
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

  print_local_addresses();

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
/*---------------------------------------------------------------------------*/
