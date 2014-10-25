#include "contiki.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include <stdio.h> /* For printf() */

/*---------------------------------------------------------------------------*/
PROCESS(samr21_demo_process, "SAM-R21 Demo Process");
AUTOSTART_PROCESSES(&samr21_demo_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(samr21_demo_process, ev, data)
{
  PROCESS_BEGIN();

  printf("SAM-R21 Demo Process started\n");

  SENSORS_ACTIVATE(button_sensor);

  while (1) {
    PROCESS_YIELD();

    if(ev == sensors_event) {
      if(data == &button_sensor && button_sensor.value(0)) {
        printf("button pressed\n");
        leds_toggle(LEDS_GREEN);
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
