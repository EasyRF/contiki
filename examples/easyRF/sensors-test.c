#include "contiki.h"
#include "log.h"
#include "dev/sensor_qtouch_wheel.h"

/*---------------------------------------------------------------------------*/
PROCESS(sensors_test_process, "Sensors-test process");
AUTOSTART_PROCESSES(&sensors_test_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensors_test_process, ev, data)
{
  struct sensors_sensor *sensor;

  PROCESS_BEGIN();

  INFO("Sensors-test started");

//  SENSORS_ACTIVATE(pressure_sensor);
//  SENSORS_ACTIVATE(rgbc_sensor);
//  SENSORS_ACTIVATE(rh_sensor);
  SENSORS_ACTIVATE(touch_wheel_sensor);

  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event);

    /* If we woke up after a sensor event, inform what happened */
    sensor = (struct sensors_sensor *)data;
    if(sensor == &touch_wheel_sensor) {
      INFO("state: %d, position = %d",
           touch_wheel_sensor.value(TOUCH_WHEEL_STATE),
           touch_wheel_sensor.value(TOUCH_WHEEL_POSITION));
    }
  }

  PROCESS_END();
}
