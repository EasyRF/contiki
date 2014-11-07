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
#include "contiki.h"
#include "contiki-net.h"
#include "http-socket.h"
#include "dev/sensor_tcs3772.h"
#include "log.h"

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"


#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])


static uint8_t sensor_data[128];
static char server_response[64];
/*---------------------------------------------------------------------------*/
PROCESS(http_post_process, "HTTP POST Process");
AUTOSTART_PROCESSES(&http_post_process);
/*---------------------------------------------------------------------------*/
void http_socket_callback(struct http_socket *s,
                          void *ptr,
                          http_socket_event_t ev,
                          const uint8_t *data,
                          uint16_t datalen)
{

  if (ev == HTTP_SOCKET_DATA) {
    memcpy(server_response, data, datalen);
    TRACE(server_response);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(http_post_process, ev, data)
{
  static struct etimer et;
  static struct http_socket hs;

  PROCESS_BEGIN();

  INFO("HTTP POST test started");

  etimer_set(&et, CLOCK_SECOND * 5);

  while (1) {
    PROCESS_WAIT_UNTIL(etimer_expired(&et));

    snprintf((char *)sensor_data, sizeof(sensor_data), "{\"red\":\"%02X\",\"green\":\"%02X\",\"blue\":\"%02X\"}",
             rgbc_sensor.value(RGBC_RED_BYTE),
             rgbc_sensor.value(RGBC_GREEN_BYTE),
             rgbc_sensor.value(RGBC_BLUE_BYTE));

    http_socket_post(&hs, "http://192.168.2.7:9999/api/devices/1",
                     sensor_data, strlen((const char *)sensor_data),
                     "application/json", http_socket_callback, 0);

//    http_socket_get(&hs, "http://192.168.2.7:9999/api/devices", http_socket_callback, 0);

    etimer_restart(&et);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
