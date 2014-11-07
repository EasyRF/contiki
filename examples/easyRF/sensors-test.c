#include "contiki.h"
#include "contiki-net.h"
#include "http-socket.h"
#include "log.h"
#include "dev/leds.h"
#include "dev/sensor_qtouch_wheel.h"
#include "dev/sensor_joystick.h"
#include "dev/sensor_bmp180.h"
#include "dev/sensor_si7020.h"
#include "dev/sensor_tcs3772.h"
#include "dev/display_st7565s.h"


#define APPLICATION_JSON  "application/json"


/*---------------------------------------------------------------------------*/
PROCESS(sensors_test_process, "Sensors-test process");
PROCESS(http_post_process, "HTTP POST Process");
AUTOSTART_PROCESSES(&sensors_test_process, &http_post_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensors_test_process, ev, data)
{
  static int cnt = 0;
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

//  displ_drv_st7565s.on();
//  displ_drv_st7565s.set_backlight(1);

  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event);

    sensor = (struct sensors_sensor *)data;

    if (sensor == &touch_wheel_sensor) {
      INFO("state: %d, position = %d",
           touch_wheel_sensor.value(TOUCH_WHEEL_STATE),
           touch_wheel_sensor.value(TOUCH_WHEEL_POSITION));
    } else if (sensor == &joystick_sensor) {
      INFO("joystick position: %d",
           joystick_sensor.value(0));
    } else if (sensor == &pressure_sensor) {
      INFO("ambient pressure: %d, temperature: %d",
           pressure_sensor.value(BMP180_PRESSURE),
           pressure_sensor.value(BMP180_TEMPERATURE));
    } else if (sensor == &rgbc_sensor) {
      INFO("red: %d, green: %d, blue: %d, clear: %d",
           rgbc_sensor.value(RGBC_RED),
           rgbc_sensor.value(RGBC_GREEN),
           rgbc_sensor.value(RGBC_BLUE),
           rgbc_sensor.value(RGBC_CLEAR));
    } else if (sensor == &rh_sensor) {
      INFO("humidity: %d, temperature: %d",
           rh_sensor.value(SI7020_HUMIDITY),
           rh_sensor.value(SI7020_TEMPERATURE));
    }

    /* Draw a pixel on the screen at each sensor change */
    displ_drv_st7565s.set_px(cnt % 128, cnt / 128, 1);
    cnt++;
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
             rgbc_sensor.value(RGBC_RED_BYTE),
             rgbc_sensor.value(RGBC_GREEN_BYTE),
             rgbc_sensor.value(RGBC_BLUE_BYTE),
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
