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
#include <stdbool.h>

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ip/resolv.h"
#include "dev/leds.h"
#include "log.h"
#include "lib/sensors.h"
#include "dev/sensor_joystick.h"


#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

#define SERVER_PORT 80

static struct tcp_socket socket;
static uint8_t connected;
static uint8_t sending;

static uip_ipaddr_t ipaddr;

#define INPUTBUFSIZE 400
static uint8_t inputbuf[INPUTBUFSIZE];

#define OUTPUTBUFSIZE 400
static uint8_t outputbuf[OUTPUTBUFSIZE];

#define SEND_INTERVAL     (CLOCK_SECOND)
#define MAX_SEND_INTERVAL (CLOCK_SECOND * 5)
#define MAX_PAYLOAD_LEN   OUTPUTBUFSIZE

static int send_interval = SEND_INTERVAL;
static int packet_length = 10; //MAX_PAYLOAD_LEN / 2;

/*---------------------------------------------------------------------------*/
PROCESS(tcp_client_process, "TCP client process");
AUTOSTART_PROCESSES(&resolv_process,&tcp_client_process);
/*---------------------------------------------------------------------------*/
static uint8_t buf[MAX_PAYLOAD_LEN];
static void
timeout_handler(void)
{
  if (!connected) {
    INFO("waiting for socket connection...");
    return;
  }

  if (sending) {
    printf(".");
    return;
  }

  buf[0]++;

  leds_toggle(LEDS_GREEN);

#if SEND_TOO_LARGE_PACKET_TO_TEST_FRAGMENTATION
  tcp_socket_send(&socket, (const uint8_t *)buf, UIP_APPDATA_SIZE);
#else /* SEND_TOO_LARGE_PACKET_TO_TEST_FRAGMENTATION */
  tcp_socket_send(&socket, (const uint8_t *)buf, packet_length);//(random_rand() % MAX_PAYLOAD_LEN) + 1);
#endif /* SEND_TOO_LARGE_PACKET_TO_TEST_FRAGMENTATION */

  sending = 1;

//  INFO("START tcp send");
}
/*---------------------------------------------------------------------------*/
static int
input(struct tcp_socket *s, void *ptr,
      const uint8_t *inputptr, int inputdatalen)
{
  printf("input %d bytes '%s'\n", inputdatalen, inputptr);
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
event(struct tcp_socket *s, void *ptr,
      tcp_socket_event_t ev)
{
  if(ev == TCP_SOCKET_CONNECTED) {
    INFO("Socket connected");
    connected = 1;
  } else if(ev == TCP_SOCKET_DATA_SENT) {
//    INFO("END tcp send");
    sending = 0;
  } else if(ev == TCP_SOCKET_CLOSED) {
    connected = 0;
    sending = 0;
    INFO("Socket closed");
  } else if(ev == TCP_SOCKET_ABORTED) {
    connected = 0;
    sending = 0;
    INFO("Socket reset");
  } else if(ev == TCP_SOCKET_TIMEDOUT) {
    connected = 0;
    sending = 0;
    INFO("Socket timedout");
  }
}
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Client IPv6 addresses: ");
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
#if UIP_CONF_ROUTER
static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;

  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
}
#endif /* UIP_CONF_ROUTER */
/*---------------------------------------------------------------------------*/
static resolv_status_t
set_connection_address(uip_ipaddr_t *ipaddr)
{
#ifndef TCP_CONNECTION_ADDR
#if RESOLV_CONF_SUPPORTS_MDNS
#define TCP_CONNECTION_ADDR       contiki-tcp-server.local
#elif UIP_CONF_ROUTER
#define TCP_CONNECTION_ADDR       aaaa:0:0:0:0212:7404:0004:0404
#else
#define TCP_CONNECTION_ADDR       fe80:0:0:0:6466:6666:6666:6666
#endif
#endif /* !TCP_CONNECTION_ADDR */

#define _QUOTEME(x) #x
#define QUOTEME(x) _QUOTEME(x)

  resolv_status_t status = RESOLV_STATUS_ERROR;

  if(uiplib_ipaddrconv(QUOTEME(TCP_CONNECTION_ADDR), ipaddr) == 0) {
    uip_ipaddr_t *resolved_addr = NULL;
    status = resolv_lookup(QUOTEME(TCP_CONNECTION_ADDR),&resolved_addr);
    if(status == RESOLV_STATUS_UNCACHED || status == RESOLV_STATUS_EXPIRED) {
      PRINTF("Attempting to look up %s\n",QUOTEME(TCP_CONNECTION_ADDR));
      resolv_query(QUOTEME(TCP_CONNECTION_ADDR));
      status = RESOLV_STATUS_RESOLVING;
    } else if(status == RESOLV_STATUS_CACHED && resolved_addr != NULL) {
      PRINTF("Lookup of \"%s\" succeded!\n",QUOTEME(TCP_CONNECTION_ADDR));
    } else if(status == RESOLV_STATUS_RESOLVING) {
      PRINTF("Still looking up \"%s\"...\n",QUOTEME(TCP_CONNECTION_ADDR));
    } else {
      PRINTF("Lookup of \"%s\" failed. status = %d\n",QUOTEME(TCP_CONNECTION_ADDR),status);
    }
    if(resolved_addr)
      uip_ipaddr_copy(ipaddr, resolved_addr);
  } else {
    status = RESOLV_STATUS_CACHED;
  }

  return status;
}
/*---------------------------------------------------------------------------*/
PROCESS(adjust_packet_length_process, "adjust_packet_length_process");
PROCESS_THREAD(adjust_packet_length_process, ev, data)
{
  struct sensors_sensor *sensor;

  PROCESS_BEGIN();

  process_start(&sensors_process, NULL);
  SENSORS_ACTIVATE(joystick_sensor);

  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event);

    sensor = (struct sensors_sensor *)data;

    if (sensor == &joystick_sensor ) {
      int new_packet_length = packet_length;
      int new_send_interval = send_interval;

      if (joystick_sensor.value(JOYSTICK_STATE) == JOYSTICK_LEFT) {
        new_packet_length -= 1;
      } else if (joystick_sensor.value(JOYSTICK_STATE) == JOYSTICK_RIGHT) {
        new_packet_length += 1;
      } else if (joystick_sensor.value(JOYSTICK_STATE) == JOYSTICK_UP) {
        new_send_interval += 10;
      } else if (joystick_sensor.value(JOYSTICK_STATE) == JOYSTICK_DOWN) {
        new_send_interval -= 10;
      }

      if (new_packet_length < 1) {
        new_packet_length = 1;
      } else if (new_packet_length > MAX_PAYLOAD_LEN) {
        new_packet_length = MAX_PAYLOAD_LEN;
      }

      if (new_packet_length != packet_length) {
        packet_length = new_packet_length;
        INFO("new packet length = %d", packet_length);
      }

      if (new_send_interval < 1) {
        new_send_interval = 1;
      } else if (new_send_interval > MAX_SEND_INTERVAL) {
        new_send_interval = MAX_SEND_INTERVAL;
      }

      if (new_send_interval != send_interval) {
        send_interval = new_send_interval;
        INFO("new send interval = %d", send_interval);
      }
    }
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(tcp_client_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

  PRINTF("TCP client process started\n");

  process_start(&adjust_packet_length_process, 0);

#if UIP_CONF_ROUTER
  set_global_address();
#endif

  print_local_addresses();

  /* Fill buffer with test data */
  for (int i = 0; i < MAX_PAYLOAD_LEN; i++) {
    buf[i] = i;
  }

  static resolv_status_t status = RESOLV_STATUS_UNCACHED;
  while(status != RESOLV_STATUS_CACHED) {
    status = set_connection_address(&ipaddr);

    if(status == RESOLV_STATUS_RESOLVING) {
      PROCESS_WAIT_EVENT_UNTIL(ev == resolv_event_found);
    } else if(status != RESOLV_STATUS_CACHED) {
      PRINTF("Can't get connection address.\n");
      PROCESS_YIELD();
    }
  }

  /* new connection with remote host */
  tcp_socket_register(&socket, NULL,
                        inputbuf, sizeof(inputbuf),
                        outputbuf, sizeof(outputbuf),
                        input, event);
  tcp_socket_connect(&socket, &ipaddr, SERVER_PORT);

  PRINTF("Connecting with the server...");
  PRINT6ADDR(&ipaddr);
  PRINTF("\n");

  while(1) {
    etimer_set(&et, send_interval);
    PROCESS_YIELD();
    if(etimer_expired(&et)) {
      timeout_handler();
    }
  }

  PROCESS_END();
}
