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
#include "log.h"
#include "dev/leds.h"
#include "dev/flash.h"
#include "dev/sensor_qtouch_wheel.h"
#include "dev/sensor_joystick.h"
#include "dev/sensor_bmp180.h"
#include "dev/sensor_si7020.h"
#include "dev/sensor_tcs3772.h"
#include "dev/display_st7565s.h"
#include "canvas_textbox.h"


#define APPLICATION_JSON  "application/json"


/*---------------------------------------------------------------------------*/
PROCESS(sensors_test_process, "Sensors-test process");
PROCESS(http_post_process, "HTTP POST Process");
AUTOSTART_PROCESSES(&sensors_test_process, &http_post_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensors_test_process, ev, data)
{
  static display_value_t width, height;
  static char text_buffer[64];
  static int verdane8_bold, verdane7;
  static struct canvas_textbox tb_wheel_position, tb_color_red;

  struct sensors_sensor *sensor;

  PROCESS_BEGIN();

  INFO("Sensors-test started");

  process_start(&sensors_process, NULL);

  rgbc_sensor.configure(TCS3772_READ_INTERVAL, CLOCK_SECOND / 2);

  SENSORS_ACTIVATE(touch_wheel_sensor);
  SENSORS_ACTIVATE(joystick_sensor);
  SENSORS_ACTIVATE(pressure_sensor);
  SENSORS_ACTIVATE(rgbc_sensor);
  SENSORS_ACTIVATE(rh_sensor);

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

  verdane8_bold = canvas_load_font("verdane8_bold_cfs.bmp");
  verdane7 = canvas_load_font("/verdane7.bmp");

  canvas_textbox_init(&tb_wheel_position, 5, width / 2, height / 2, 11,
                   DISPLAY_COLOR_BLACK, DISPLAY_COLOR_WHITE, DISPLAY_COLOR_BLACK);

  canvas_textbox_init(&tb_color_red, 5, width / 2, tb_wheel_position.rect.top + tb_wheel_position.rect.height + 2, 10,
                   DISPLAY_COLOR_WHITE, DISPLAY_COLOR_BLACK, DISPLAY_COLOR_BLACK);

  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event);

    sensor = (struct sensors_sensor *)data;

    if (sensor == &touch_wheel_sensor) {
      INFO("state: %d, position = %d",
           touch_wheel_sensor.value(TOUCH_WHEEL_STATE),
           touch_wheel_sensor.value(TOUCH_WHEEL_POSITION));

      snprintf(text_buffer, sizeof(text_buffer), "Wheel: %d",
               touch_wheel_sensor.value(TOUCH_WHEEL_POSITION));

      canvas_textbox_draw_string_reset(&display_st7565s, &tb_wheel_position, verdane8_bold, text_buffer);

    } else if (sensor == &joystick_sensor) {
      INFO("joystick position: %d",
           joystick_sensor.value(0));
    } else if (sensor == &pressure_sensor) {
      INFO("ambient pressure: %d, temperature: %d",
           pressure_sensor.value(BMP180_PRESSURE),
           pressure_sensor.value(BMP180_TEMPERATURE));
    } else if (sensor == &rgbc_sensor) {
      INFO("red: %d, green: %d, blue: %d, clear: %d",
           rgbc_sensor.value(TCS3772_RED),
           rgbc_sensor.value(TCS3772_GREEN),
           rgbc_sensor.value(TCS3772_BLUE),
           rgbc_sensor.value(TCS3772_CLEAR));

      snprintf(text_buffer, sizeof(text_buffer), "Red: %d",
               rgbc_sensor.value(TCS3772_RED));
      canvas_textbox_draw_string_reset(&display_st7565s, &tb_color_red, verdane7, text_buffer);
    } else if (sensor == &rh_sensor) {
      INFO("humidity: %d, temperature: %d",
           rh_sensor.value(SI7020_HUMIDITY),
           rh_sensor.value(SI7020_TEMPERATURE));
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
    INFO("Response from the server: '%s'\n", str);

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
  static char sensor_data[128];

  PROCESS_BEGIN();

  etimer_set(&et, CLOCK_SECOND * 1);

  while (1) {
    PROCESS_WAIT_UNTIL(etimer_expired(&et));

    uint32_t red   = rgbc_sensor.value(TCS3772_RED);
    uint32_t green = rgbc_sensor.value(TCS3772_GREEN);
    uint32_t blue  = rgbc_sensor.value(TCS3772_BLUE);

    uint32_t rgb_max = max(red, max(green, blue));

    snprintf(sensor_data, sizeof(sensor_data),
             "{"
               "\"red\":\"%02X\","
               "\"green\":\"%02X\","
               "\"blue\":\"%02X\","
               "\"pressure\":%d,"
               "\"temperature\":%d,"
               "\"humidity\":%d,"
               "\"joystick\":\"%s\","
               "\"wheel\":%d"
             "}",
             (uint8_t)(red   * 255 / rgb_max),
             (uint8_t)(green * 255 / rgb_max),
             (uint8_t)(blue  * 255 / rgb_max),
             pressure_sensor.value(BMP180_PRESSURE),
             pressure_sensor.value(BMP180_TEMPERATURE),
             rh_sensor.value(SI7020_HUMIDITY),
             JOYSTICK_STATE_TO_STRING(joystick_sensor.value(JOYSTICK_STATE)),
             touch_wheel_sensor.value(TOUCH_WHEEL_POSITION)
    );

    http_socket_post(&hs, "http://192.168.2.7:9999/api/devices/1",
                     (const uint8_t *)sensor_data, strlen((const char *)sensor_data),
                     APPLICATION_JSON, http_socket_callback, 0);

    etimer_restart(&et);
  }

  PROCESS_END();
}
