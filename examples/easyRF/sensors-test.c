#include "contiki.h"
#include "log.h"
#include "dev/sensor_qtouch_wheel.h"
#include "dev/sensor_joystick.h"
#include "dev/sensor_bmp180.h"
#include "dev/sensor_si7020.h"
#include "dev/sensor_tcs3772.h"
#include "dev/display_st7565s.h"


/*---------------------------------------------------------------------------*/
PROCESS(sensors_test_process, "Sensors-test process");
AUTOSTART_PROCESSES(&sensors_test_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensors_test_process, ev, data)
{
  static int cnt = 0;
  struct sensors_sensor *sensor;

  PROCESS_BEGIN();

  INFO("Sensors-test started");

  SENSORS_ACTIVATE(touch_wheel_sensor);
  SENSORS_ACTIVATE(joystick_sensor);
  SENSORS_ACTIVATE(pressure_sensor);
  SENSORS_ACTIVATE(rgbc_sensor);
  SENSORS_ACTIVATE(rh_sensor);

  displ_drv_st7565s.on();
  displ_drv_st7565s.set_backlight(1);

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

    displ_drv_st7565s.set_px(cnt % 128, cnt / 128, 1);
    cnt++;
  }

  PROCESS_END();
}
