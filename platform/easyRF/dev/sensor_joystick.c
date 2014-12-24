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

#include <asf.h>
#include "dev/sensor_joystick.h"
#include "log.h"

#undef TRACE
#define TRACE(...)


/* Reference voltage (mV) */
#define ADC_REF_VOLTAGE     2230
#define ADC_MAX_VALUE       4095

/* Voltage (mV) for all joystick states/positions */
#define IDLE_VOLTAGE        10
#define UP_VOLTAGE          1101
#define RIGHT_VOLTAGE       925
#define DOWN_VOLTAGE        1652
#define LEFT_VOLTAGE        537
#define BUTTON_VOLTAGE      2230

/* Compare macros */
#define VOLTAGE_MARGIN        25
#define VAL_EQUALS_POS(i,r)   (abs((int16_t)(i) - (int16_t)(r)) < VOLTAGE_MARGIN)
#define UP(v)                 VAL_EQUALS_POS(v, UP_VOLTAGE)
#define RIGHT(v)              VAL_EQUALS_POS(v, RIGHT_VOLTAGE)
#define DOWN(v)               VAL_EQUALS_POS(v, DOWN_VOLTAGE)
#define LEFT(v)               VAL_EQUALS_POS(v, LEFT_VOLTAGE)
#define BUTTON_PRESSED(v)     VAL_EQUALS_POS(v, BUTTON_VOLTAGE)

/* Convert ADC value to voltage (mV) */
#define ADC_TO_MV(x)          ((uint32_t)x * ADC_REF_VOLTAGE / ADC_MAX_VALUE)

/* Update interval */
#define JOYSTICK_DEFAULT_READ_INTERVAL    (CLOCK_SECOND / 100)


static struct adc_module adc_instance;
static uint8_t joystick_state;
static bool sensor_active;
static struct etimer read_timer;

const char * joystick_state_strings[] = {
  "Idle","Up","Right","Down","Left","Button"
};

/*---------------------------------------------------------------------------*/
PROCESS(joystick_process, "Joystick Process");
/*---------------------------------------------------------------------------*/
static bool
update(void)
{
  uint16_t value, voltage;
  uint8_t new_state;

  /* Read raw ADC value */
  if (adc_read(&adc_instance, &value) != STATUS_OK) {
    return false;
  }

  /* Start new conversion */
  adc_start_conversion(&adc_instance);

  /* Convert value to millivolts */
  voltage = ADC_TO_MV(value);

  if (UP(voltage)) {
    new_state = JOYSTICK_UP;
  } else if (RIGHT(voltage)) {
    new_state = JOYSTICK_RIGHT;
  } else if (DOWN(voltage)) {
    new_state = JOYSTICK_DOWN;
  } else if (LEFT(voltage)) {
    new_state = JOYSTICK_LEFT;
  } else if (BUTTON_PRESSED(voltage)) {
    new_state = JOYSTICK_BUTTON;
  } else {
    new_state = JOYSTICK_IDLE;
  }

  if (new_state != joystick_state) {
    joystick_state = new_state;
    TRACE("new joystick state: %d (%d)", joystick_state, voltage);
    sensors_changed(&joystick_sensor);
  }

  return true;
}
/*---------------------------------------------------------------------------*/
static void
configure_adc(void)
{
  /* Initialize ADC using PIN_PB02 with 2230 mV reference */
  struct adc_config config_adc;
  adc_get_config_defaults(&config_adc);
  config_adc.positive_input = ADC_POSITIVE_INPUT_PIN10;
  config_adc.reference = ADC_REFERENCE_INTVCC0;
  config_adc.clock_prescaler = ADC_CLOCK_PRESCALER_DIV512;
  adc_init(&adc_instance, ADC, &config_adc);

  /* Start new conversion */
  adc_start_conversion(&adc_instance);

  /* Log */
  TRACE("ADC configured");
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  switch (type) {
  case JOYSTICK_STATE:
    return joystick_state;
  default:
    WARN("Unknown value type");
    return -1;
  }
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  switch(type) {
  case SENSORS_ACTIVE:
  case SENSORS_READY:
    return sensor_active;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int value)
{
  switch(type) {
  case SENSORS_HW_INIT:
    configure_adc();
    etimer_set(&read_timer, JOYSTICK_DEFAULT_READ_INTERVAL);
    return 1;
  case SENSORS_ACTIVE:
    if (value) {
      adc_enable(&adc_instance);
      process_start(&joystick_process, 0);
      sensor_active = 1;
    } else {
      adc_disable(&adc_instance);
      process_exit(&joystick_process);
      sensor_active = 0;
    }
    return 1;
  case JOYSTICK_READ_INTERVAL:
    etimer_set(&read_timer, value);
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(joystick_process, ev, data)
{
  PROCESS_BEGIN();

  while(1) {
    etimer_restart(&read_timer);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&read_timer));

    update();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(joystick_sensor, "Joystick Sensor", value, configure, status);
