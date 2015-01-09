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


#define APPLICATION_JSON        "application/json"
#define SERVER_URL              "http://192.168.1.36:9999/api/devices/"

#define MS2TICKS(ms)            ((int)((uint32_t) CLOCK_SECOND * ms / 1000))


static signed short rssi;

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
PROCESS_THREAD(sensors_test_process, ev, data)
{
  static display_value_t width, height;
  static char text_buffer[64];
  static int verdane7;
  static struct canvas_textbox tb_wheel_position, tb_color, tb_joystick;

  struct sensors_sensor *sensor;

  PROCESS_BEGIN();

  INFO("Sensors-test started");

  /* Add sniffer on received packets to extract the RSSI value */
  rime_sniffer_add(&packetsniff);

  process_start(&sensors_process, NULL);

  rgbc_sensor.configure(TCS3772_READ_INTERVAL,  MS2TICKS(TCS3772_CYCLE_MS) );

  SENSORS_ACTIVATE(touch_wheel_sensor);
  SENSORS_ACTIVATE(joystick_sensor);
  SENSORS_ACTIVATE(pressure_sensor);
  SENSORS_ACTIVATE(rgbc_sensor);
  SENSORS_ACTIVATE(rh_sensor);
  SENSORS_ACTIVATE(nineaxis_sensor);

  pressure_sensor.configure(BMP180_READ_INTERVAL, CLOCK_SECOND / 2);

  display_st7565s.on();
  display_st7565s.set_value(DISPLAY_BACKLIGHT, 1);
  display_st7565s.set_value(DISPLAY_FLIP_X, 1);
  display_st7565s.set_value(DISPLAY_FLIP_Y, 1);
  display_st7565s.get_value(DISPLAY_WIDTH, &width);
  display_st7565s.get_value(DISPLAY_HEIGHT, &height);

  /* Open external flash */
  EXTERNAL_FLASH.open();

  struct canvas_point p;
  p.x = 0; p.y = 0;
  canvas_draw_bmp(&display_st7565s, "/logo_easyrf.bmp", &p,
                  DISPLAY_COLOR_BLACK, DISPLAY_COLOR_WHITE);

  verdane7 = canvas_load_font("/verdane7.bmp");

  canvas_textbox_init(&tb_wheel_position, 0, width, 26, 10,
                      DISPLAY_COLOR_BLACK, DISPLAY_COLOR_WHITE, DISPLAY_COLOR_WHITE);

  canvas_textbox_init(&tb_joystick, 0, width, 36, 10,
                      DISPLAY_COLOR_BLACK, DISPLAY_COLOR_WHITE, DISPLAY_COLOR_WHITE);

  canvas_textbox_init(&tb_color, 0, width, 46, 10,
                      DISPLAY_COLOR_BLACK, DISPLAY_COLOR_WHITE, DISPLAY_COLOR_WHITE);

  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event);

    sensor = (struct sensors_sensor *)data;

    if (sensor == &touch_wheel_sensor) {
      snprintf(text_buffer, sizeof(text_buffer), "Whl: %d",
               touch_wheel_sensor.value(TOUCH_WHEEL_POSITION));
      canvas_textbox_draw_string_reset(&display_st7565s, &tb_wheel_position, verdane7, text_buffer);
    } else if (sensor == &joystick_sensor) {
      snprintf(text_buffer, sizeof(text_buffer), "Joy: %s",
               JOYSTICK_STATE_TO_STRING(joystick_sensor.value(JOYSTICK_STATE)));
      canvas_textbox_draw_string_reset(&display_st7565s, &tb_joystick, verdane7, text_buffer);
    } else if (sensor == &pressure_sensor) {
//      INFO("ambient pressure: %d, temperature: %d",
//           pressure_sensor.value(BMP180_PRESSURE),
//           pressure_sensor.value(BMP180_TEMPERATURE));
    } else if (sensor == &rgbc_sensor) {
      snprintf(text_buffer, sizeof(text_buffer), "Clr: %02X.%02X.%02X",
               rgbc_sensor.value(TCS3772_RED),
               rgbc_sensor.value(TCS3772_GREEN),
               rgbc_sensor.value(TCS3772_BLUE));
      canvas_textbox_draw_string_reset(&display_st7565s, &tb_color, verdane7, text_buffer);
    } else if (sensor == &rh_sensor) {
//      INFO("humidity: %d, temperature: %d",
//           rh_sensor.value(SI7020_HUMIDITY),
//           rh_sensor.value(SI7020_TEMPERATURE));
    } else if (sensor == &nineaxis_sensor) {
//      INFO("GYRO %6d,%6d,%6d ACC %6d,%6d,%6d COMPASS %6d,%6d,%6d (T %d)",
//           nineaxis_sensor.value(LSM9DS1_GYRO_X),
//           nineaxis_sensor.value(LSM9DS1_GYRO_Y),
//           nineaxis_sensor.value(LSM9DS1_GYRO_Z),
//           nineaxis_sensor.value(LSM9DS1_ACC_X),
//           nineaxis_sensor.value(LSM9DS1_ACC_Y),
//           nineaxis_sensor.value(LSM9DS1_ACC_Z),
//           nineaxis_sensor.value(LSM9DS1_COMPASS_X),
//           nineaxis_sensor.value(LSM9DS1_COMPASS_Y),
//           nineaxis_sensor.value(LSM9DS1_COMPASS_Z),
//           nineaxis_sensor.value(LSM9DS1_TEMP));
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void http_socket_callback(struct http_socket *s,
                          void *ptr,
                          http_socket_event_t ev,
                          const uint8_t *data,
                          uint16_t datalen)
{
  char *str;

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
  static struct etimer et;
  static struct http_socket hs;
  static char sensor_data[512];

  static char parent_addr[64];
  static char server_url[128];

  uip_ds6_addr_t *ds6addr;
  const uip_ipaddr_t *parent;
  rpl_dag_t *dag;

  PROCESS_BEGIN();

  ds6addr = uip_ds6_get_global(ADDR_PREFERRED);
  snprintf(server_url, sizeof(server_url),
           "%s%02x%02x:%02x%02x:%02x%02x:%02x%02x",
           SERVER_URL,
           ds6addr->ipaddr.u8[8], ds6addr->ipaddr.u8[9],
           ds6addr->ipaddr.u8[10], ds6addr->ipaddr.u8[11],
           ds6addr->ipaddr.u8[12], ds6addr->ipaddr.u8[13],
           ds6addr->ipaddr.u8[14], ds6addr->ipaddr.u8[15]);

  etimer_set(&et, CLOCK_SECOND * 1);

  while (1) {
    PROCESS_WAIT_UNTIL(etimer_expired(&et));

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
             "\"rssi\":%d,"
             "\"p\":\"%s\","
             "\"red\":%d,"
             "\"green\":%d,"
             "\"blue\":%d,"
             "\"proximity\":%d,"
             "\"pressure\":%d,"
             "\"temperature\":%d,"
             "\"humidity\":%d,"
             "\"joystick\":\"%s\","
             "\"wheel\":%d,"
             "\"gyro_x\":%d,"
             "\"gyro_y\":%d,"
             "\"gyro_z\":%d,"
             "\"acceleration_x\":%d,"
             "\"acceleration_y\":%d,"
             "\"acceleration_z\":%d,"
             "\"compass_x\":%d,"
             "\"compass_y\":%d,"
             "\"compass_z\":%d"
             "}",
             rssi,
             parent_addr,
             (uint8_t)(red   * 255 / rgb_max),
             (uint8_t)(green * 255 / rgb_max),
             (uint8_t)(blue  * 255 / rgb_max),
             rgbc_sensor.value         (TCS3772_PROX),
             pressure_sensor.value     (BMP180_PRESSURE),
             pressure_sensor.value     (BMP180_TEMPERATURE),
             rh_sensor.value           (SI7020_HUMIDITY),
             JOYSTICK_STATE_TO_STRING  (joystick_sensor.value(JOYSTICK_STATE)),
             touch_wheel_sensor.value  (TOUCH_WHEEL_POSITION),
             nineaxis_sensor.value     (LSM9DS1_GYRO_X),
             nineaxis_sensor.value     (LSM9DS1_GYRO_Y),
             nineaxis_sensor.value     (LSM9DS1_GYRO_Z),
             nineaxis_sensor.value     (LSM9DS1_ACC_X),
             nineaxis_sensor.value     (LSM9DS1_ACC_Y),
             nineaxis_sensor.value     (LSM9DS1_ACC_Z),
             nineaxis_sensor.value     (LSM9DS1_COMPASS_X),
             nineaxis_sensor.value     (LSM9DS1_COMPASS_Y),
             nineaxis_sensor.value     (LSM9DS1_COMPASS_Z)
             );

    http_socket_post(&hs, server_url,
                     (const uint8_t *)sensor_data, strlen((const char *)sensor_data),
                     APPLICATION_JSON, http_socket_callback, 0);

    etimer_restart(&et);
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
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
output_packetsniffer(int mac_status)
{
}
