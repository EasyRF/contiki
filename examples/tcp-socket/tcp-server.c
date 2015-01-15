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

#include <string.h>
#include "compiler.h"
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "dev/leds.h"
#include "log.h"

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

#define MAX_PAYLOAD_LEN 120

#define SERVER_PORT     80

static struct tcp_socket socket;

#define INPUTBUFSIZE 400
static uint8_t inputbuf[INPUTBUFSIZE];

#define OUTPUTBUFSIZE 400
static uint8_t outputbuf[OUTPUTBUFSIZE];


#define MAX_CLIENTS 10
struct client_data {
  uip_ipaddr_t src_addr;
  uint8_t last_seq_id;
  bool used;
};
static struct client_data client_data[MAX_CLIENTS];

PROCESS(tcp_server_process, "TCP server process");
AUTOSTART_PROCESSES(&resolv_process,&tcp_server_process);
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
static int
input(struct tcp_socket *s, void *ptr,
      const uint8_t *inputptr, int inputdatalen)
{
  if (inputdatalen > 1) {
    struct client_data * data = get_or_create_client_data(&s->c->ripaddr);
    if (!data) {
      WARN("No more room for clients");
      return 0;
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
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
static void
event(struct tcp_socket *s, void *ptr, tcp_socket_event_t ev)
{
  if(ev == TCP_SOCKET_CONNECTED) {
    printf("Socket connected\n");
  } else if(ev == TCP_SOCKET_DATA_SENT) {
    printf("Socket data was sent\n");
  } else if(ev == TCP_SOCKET_CLOSED) {
    printf("Socket closed\n");
  } else if(ev == TCP_SOCKET_ABORTED) {
    printf("Socket reset\n");
  } else if(ev == TCP_SOCKET_TIMEDOUT) {
    printf("Socket timedout\n");
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
PROCESS_THREAD(tcp_server_process, ev, data)
{
#if UIP_CONF_ROUTER
  uip_ipaddr_t ipaddr;
#endif /* UIP_CONF_ROUTER */

  PROCESS_BEGIN();
  PRINTF("TCP server started\n");

#if RESOLV_CONF_SUPPORTS_MDNS
  resolv_set_hostname("contiki-tcp-server");
#endif

#if UIP_CONF_ROUTER
  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
#endif /* UIP_CONF_ROUTER */

  print_local_addresses();

  tcp_socket_register(&socket, NULL,
                 inputbuf, sizeof(inputbuf),
                 outputbuf, sizeof(outputbuf),
                 input, event);
  tcp_socket_listen(&socket, SERVER_PORT);
  printf("Listening on %d\n", SERVER_PORT);

  while(1) {
    PROCESS_YIELD();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
