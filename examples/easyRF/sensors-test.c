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

#include "compiler.h"
#include "contiki.h"
#include "contiki-net.h"
#include "http-socket.h"
#include "rpl.h"
#include "simple-rpl.h"
#include "log.h"
#include "dev/leds.h"
#include "dev/flash.h"
#include "dev/sensor_qtouch_wheel.h"
#include "dev/sensor_joystick.h"
#include "dev/sensor_bmp180.h"
#include "dev/sensor_si7020.h"
#include "dev/sensor_tcs3772.h"
#include "dev/sensor_lsm9ds1.h"
#include "dev/display_st7565s.h"
#include "canvas_textbox.h"
#include "stack_test.h"


#define APPLICATION_JSON        "application/json"
#define SERVER_URL              "http://192.168.1.101:9999/api/devices/"
#define HTTP_POST_INTERVAL      (CLOCK_SECOND * 2)

#define MS2TICKS(ms)            ((int)((uint32_t) CLOCK_SECOND * (ms) / 1000))
#define TICKS2MS(ticks)         ((uint32_t)(ticks) * 1000 / CLOCK_SECOND)

enum display_page {
  SENSOR_PAGE_1,
  SENSOR_PAGE_2,
  NETWORK_PAGE,
  LAST_PAGE
};

static enum display_page current_page;

static char text_buffer[128];
static int text_font, header_font;

static struct canvas_textbox tb_header, tb_labels, tb_values;

static signed short rssi, lqi;

static bool http_post_enabled;
static bool http_post_in_progress;
static clock_time_t http_post_start_time;

/*---------------------------------------------------------------------------*/
static void input_packetsniffer(void);
static void output_packetsniffer(int mac_status);
RIME_SNIFFER(packetsniff, input_packetsniffer, output_packetsniffer);
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS(sensors_test_process, "Sensors-test process");
PROCESS(http_post_process, "HTTP POST Process");
AUTOSTART_PROCESSES(&sensors_test_process, &http_post_process);
/*---------------------------------------------------------------------------*/
static void
update_values(void)
{
  if (current_page == SENSOR_PAGE_1) {
    int temp = pressure_sensor.value(BMP180_TEMPERATURE);
    int temp_before_sep = temp / 10;
    int temp_after_sep  = temp - (temp_before_sep * 10);

    snprintf(text_buffer, sizeof(text_buffer), "%d.%d.%d\n%d\n%s\n%d\n%d.%d\n%d",
              (int16_t)rgbc_sensor.value(TCS3772_RED),
              (int16_t)rgbc_sensor.value(TCS3772_GREEN),
              (int16_t)rgbc_sensor.value(TCS3772_BLUE),
              (uint8_t)touch_wheel_sensor.value(TOUCH_WHEEL_POSITION),
              JOYSTICK_STATE_TO_STRING(joystick_sensor.value(JOYSTICK_STATE)),
              pressure_sensor.value(BMP180_PRESSURE),
              temp_before_sep, temp_after_sep,
              rh_sensor.value(SI7020_HUMIDITY));
  } else if (current_page == SENSOR_PAGE_2) {
    snprintf(text_buffer, sizeof(text_buffer), "%d.%d.%d\n%d.%d.%d\n%d.%d.%d",
              (int16_t)nineaxis_sensor.value(LSM9DS1_GYRO_X),
              (int16_t)nineaxis_sensor.value(LSM9DS1_GYRO_Y),
              (int16_t)nineaxis_sensor.value(LSM9DS1_GYRO_Z),
              (int16_t)nineaxis_sensor.value(LSM9DS1_ACC_X),
              (int16_t)nineaxis_sensor.value(LSM9DS1_ACC_Y),
              (int16_t)nineaxis_sensor.value(LSM9DS1_ACC_Z),
              (int16_t)nineaxis_sensor.value(LSM9DS1_COMPASS_X),
              (int16_t)nineaxis_sensor.value(LSM9DS1_COMPASS_Y),
              (int16_t)nineaxis_sensor.value(LSM9DS1_COMPASS_Z));
  } else if (current_page == NETWORK_PAGE) {
    snprintf(text_buffer, sizeof(text_buffer), "%d\n%d",
             rssi, lqi);
  }

  canvas_textbox_draw_string_reset(&display_st7565s, &tb_values, text_font,
                                   text_buffer);
}
/*---------------------------------------------------------------------------*/
static void
show_page(enum display_page page)
{
  current_page = page;

  if (current_page == SENSOR_PAGE_1) {
    canvas_textbox_draw_string_reset(&display_st7565s, &tb_header, header_font,
                                     "       Sensors 1");

    canvas_textbox_draw_string_reset(&display_st7565s, &tb_labels, text_font,
                                     "Color:\nWheel:\nJoystick:\nPressure:\nTemp:\nHumidity:");
  } else if (current_page == SENSOR_PAGE_2) {
    canvas_textbox_draw_string_reset(&display_st7565s, &tb_header, header_font,
                                     "       Sensors 2");

    canvas_textbox_draw_string_reset(&display_st7565s, &tb_labels, text_font,
                                     "Gyro:\nAccel:\nCompass:");
  } else if (current_page == NETWORK_PAGE) {
    canvas_textbox_draw_string_reset(&display_st7565s, &tb_header, header_font,
                                     "        Network");

    canvas_textbox_draw_string_reset(&display_st7565s, &tb_labels, text_font,
                                     "RSSI:\nLQI:");
  }

  update_values();
}
/*---------------------------------------------------------------------------*/
static inline void
handle_joystick_event(int joystick_state)
{
  enum display_page new_page = current_page;

  if (joystick_state == JOYSTICK_LEFT) {
    new_page = current_page - 1;
  } else if (joystick_state == JOYSTICK_RIGHT) {
    new_page = current_page + 1;
  } else if (joystick_state == JOYSTICK_BUTTON) {
    http_post_enabled = !http_post_enabled;
  }

  if (new_page < 0) {
    new_page = LAST_PAGE - 1;
  } else if (new_page == LAST_PAGE) {
    new_page = 0;
  }

  if (new_page != current_page) {
    show_page(new_page);
  }

  print_stack_info();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensors_test_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

  INFO("Sensors-test started");

  /* Add sniffer on received packets to extract the RSSI value */
  rime_sniffer_add(&packetsniff);

  process_start(&sensors_process, NULL);

  SENSORS_ACTIVATE(joystick_sensor);
//  SENSORS_ACTIVATE(touch_wheel_sensor);
//  SENSORS_ACTIVATE(pressure_sensor);
//  SENSORS_ACTIVATE(rgbc_sensor);
//  SENSORS_ACTIVATE(rh_sensor);
//  SENSORS_ACTIVATE(nineaxis_sensor);

  rgbc_sensor.configure(TCS3772_READ_INTERVAL,  MS2TICKS(TCS3772_CYCLE_MS) );
  pressure_sensor.configure(BMP180_READ_INTERVAL, CLOCK_SECOND / 2);

  display_st7565s.on();
  display_st7565s.set_value(DISPLAY_BACKLIGHT, 1);
  display_st7565s.set_value(DISPLAY_FLIP_X, 1);
  display_st7565s.set_value(DISPLAY_FLIP_Y, 1);

  /* Load header and text fonts */
  text_font = canvas_load_font("/verdane7.bmp");
  header_font = canvas_load_font("/verdane10_bold.bmp");

  /* Initialize textbox for header */
  canvas_textbox_init(&tb_header, 0, 128, 0, 10,
                      DISPLAY_COLOR_BLACK,
                      DISPLAY_COLOR_WHITE,
                      DISPLAY_COLOR_WHITE);

  /* Create header line */
  struct canvas_rectangle header_line_rect = { 0, 11, 128, 2 };

  /* Initialize textbox for labels */
  canvas_textbox_init(&tb_labels, 0, 50, 15, 64 - 15,
                      DISPLAY_COLOR_BLACK,
                      DISPLAY_COLOR_WHITE,
                      DISPLAY_COLOR_WHITE);

  /* Initialize textbox for values */
  canvas_textbox_init(&tb_values, 52, 128 - 52, 15, 64 - 15,
                      DISPLAY_COLOR_BLACK,
                      DISPLAY_COLOR_WHITE,
                      DISPLAY_COLOR_WHITE);

  /* Open external flash */
//  EXTERNAL_FLASH.open();

  /* Draw easyRF logo */
  struct canvas_point p = {0, 12};
  canvas_draw_bmp(&display_st7565s, "/logo_easyrf.bmp", &p,
                  DISPLAY_COLOR_BLACK, DISPLAY_COLOR_WHITE);

  /* Wait 5 seconds for showing splash screen */
//  etimer_set(&et, CLOCK_SECOND * 5);
//  PROCESS_WAIT_UNTIL(etimer_expired(&et));

  /* Clear screen */
  display_st7565s.clear();

  /* Draw header line */
  canvas_draw_rect(&display_st7565s, &header_line_rect,
                   DISPLAY_COLOR_BLACK, DISPLAY_COLOR_BLACK);

  /* Show sensor page at first */
  show_page(SENSOR_PAGE_1);

  while (1) {

    etimer_set(&et, CLOCK_SECOND / 2);

    PROCESS_WAIT_EVENT();

    if (ev == sensors_event) {
      struct sensors_sensor *sensor = (struct sensors_sensor *)data;

      if (sensor == &joystick_sensor) {
        handle_joystick_event(joystick_sensor.value(JOYSTICK_STATE));
      }
    }

    update_values();
  }

  PROCESS_END();
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
      http_post_enabled = true;
      INFO("Got first RPL route (num routes = %d)", numroutes);
    } else {
      INFO("Got RPL route (num routes = %d)", numroutes);
    }
  }
}
/*---------------------------------------------------------------------------*/
void http_socket_callback(struct http_socket *s,
                          void *ptr,
                          http_socket_event_t ev,
                          const uint8_t *data,
                          uint16_t datalen)
{
  char *str;

  clock_time_t http_post_stop_time = clock_time();

  INFO("FINISH HTTP POST (status = %d, duration = %ld)", ev, TICKS2MS(http_post_stop_time - http_post_start_time));

  if (ev == HTTP_SOCKET_CLOSED ||
      ev == HTTP_SOCKET_TIMEDOUT ||
      ev == HTTP_SOCKET_ABORTED) {
    http_post_in_progress = false;
  }

  if (ev == HTTP_SOCKET_DATA) {
    str = (char *)data;
    str[datalen] = '\0';
//    INFO("Response from the server: '%s'\n", str);

    if (datalen > 0) {
      char ledState = str[datalen - 1];
      if (ledState == '1') {
        leds_on(LEDS_GREEN);
      } else {
        leds_off(LEDS_GREEN);
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(http_post_process, ev, data)
{
  static struct uip_ds6_notification n;
  static struct etimer et;
  static struct http_socket hs;
  static char sensor_data[512];

  static char parent_addr[64];
  static char server_url[128];

  uip_ds6_addr_t *ds6addr;
  const uip_ipaddr_t *parent;
  rpl_dag_t *dag;

  PROCESS_BEGIN();

  uip_ds6_notification_add(&n, rpl_route_callback);

  http_post_enabled = false;
  http_post_in_progress = false;

  ds6addr = uip_ds6_get_global(ADDR_PREFERRED);
  snprintf(server_url, sizeof(server_url),
           "%s%02x%02x:%02x%02x:%02x%02x:%02x%02x",
           SERVER_URL,
           ds6addr->ipaddr.u8[8], ds6addr->ipaddr.u8[9],
           ds6addr->ipaddr.u8[10], ds6addr->ipaddr.u8[11],
           ds6addr->ipaddr.u8[12], ds6addr->ipaddr.u8[13],
           ds6addr->ipaddr.u8[14], ds6addr->ipaddr.u8[15]);

  while (1) {
    etimer_set(&et, HTTP_POST_INTERVAL);
    PROCESS_WAIT_UNTIL(etimer_expired(&et));

    if (!http_post_enabled) {
      continue;
    }

    if (http_post_in_progress) {
      continue;
    }

    dag = rpl_get_any_dag();
    parent = simple_rpl_parent();
    if (parent != NULL && dag != NULL) {
      snprintf(parent_addr, sizeof(parent_addr),
               "%04x:%04x:%04x:%04x",
               uip_htons(parent->u16[4]), uip_htons(parent->u16[5]),
               uip_htons(parent->u16[6]), uip_htons(parent->u16[7]));
    } else {
      memset(parent_addr, 0, sizeof(parent_addr));
    }

    uint32_t red   = rgbc_sensor.value(TCS3772_RED);
    uint32_t green = rgbc_sensor.value(TCS3772_GREEN);
    uint32_t blue  = rgbc_sensor.value(TCS3772_BLUE);

    uint32_t rgb_max = max(red, max(green, blue));

    snprintf(sensor_data, sizeof(sensor_data),
             "{"
             "\"rssi\":%d"
             ",\"p\":\"%s\""
             ",\"red\":%d"
             ",\"green\":%d"
             ",\"blue\":%d"
         #if 1
             ",\"proximity\":%d"
             ",\"pressure\":%d"
             ",\"temperature\":%d"
             ",\"humidity\":%d"
             ",\"joystick\":\"%s\""
             ",\"wheel\":%d"
             ",\"gyro_x\":%d"
             ",\"gyro_y\":%d"
             ",\"gyro_z\":%d"
             ",\"acceleration_x\":%d"
             ",\"acceleration_y\":%d"
             ",\"acceleration_z\":%d"
             ",\"compass_x\":%d"
             ",\"compass_y\":%d"
             ",\"compass_z\":%d"
         #endif
             "}",
             rssi
             ,parent_addr
             ,(uint8_t)(red   * 255 / rgb_max)
             ,(uint8_t)(green * 255 / rgb_max)
             ,(uint8_t)(blue  * 255 / rgb_max)
             ,rgbc_sensor.value         (TCS3772_PROX)
             ,pressure_sensor.value     (BMP180_PRESSURE)
             ,pressure_sensor.value     (BMP180_TEMPERATURE)
             ,rh_sensor.value           (SI7020_HUMIDITY)
             ,JOYSTICK_STATE_TO_STRING  (joystick_sensor.value(JOYSTICK_STATE))
             ,touch_wheel_sensor.value  (TOUCH_WHEEL_POSITION)
             ,nineaxis_sensor.value     (LSM9DS1_GYRO_X)
             ,nineaxis_sensor.value     (LSM9DS1_GYRO_Y)
             ,nineaxis_sensor.value     (LSM9DS1_GYRO_Z)
             ,nineaxis_sensor.value     (LSM9DS1_ACC_X)
             ,nineaxis_sensor.value     (LSM9DS1_ACC_Y)
             ,nineaxis_sensor.value     (LSM9DS1_ACC_Z)
             ,nineaxis_sensor.value     (LSM9DS1_COMPASS_X)
             ,nineaxis_sensor.value     (LSM9DS1_COMPASS_Y)
             ,nineaxis_sensor.value     (LSM9DS1_COMPASS_Z)
             );

    http_socket_post(&hs, server_url,
                     (const uint8_t *)sensor_data, strlen((const char *)sensor_data),
                     APPLICATION_JSON, http_socket_callback, 0);

    http_post_start_time = clock_time();

    INFO("START HTTP POST");

    http_post_in_progress = true;
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static void
input_packetsniffer(void)
{
  const uip_ipaddr_t *parent;
  const uip_lladdr_t *lladdr;

  parent = simple_rpl_parent();
  if(parent != NULL) {
    lladdr = uip_ds6_nbr_lladdr_from_ipaddr(parent);
    if(lladdr != NULL && linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_SENDER),
                                      (linkaddr_t *)lladdr)) {
      rssi = (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI);
      lqi = (signed short)packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY);
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
output_packetsniffer(int mac_status)
{
}
