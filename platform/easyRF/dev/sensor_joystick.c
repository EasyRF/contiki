#include <asf.h>
#include "dev/sensor_joystick.h"
#include "log.h"

#undef TRACE
#define TRACE(...)


/* Reference voltage (mV) */
#define ADC_REF_VOLTAGE     1650
#define ADC_MAX_VALUE       4095

/* Voltage (mV) for all joystick states/positions */
#define IDLE_VOLTAGE        1000
#define UP_VOLTAGE          804
#define RIGHT_VOLTAGE       761
#define DOWN_VOLTAGE        610
#define LEFT_VOLTAGE        896
#define BUTTON_VOLTAGE      0

/* Compare macros */
#define VOLTAGE_MARGIN        25
#define VAL_EQUALS_POS(i,r)   (abs(i - r) < VOLTAGE_MARGIN)
#define UP(v)                 VAL_EQUALS_POS(v, UP_VOLTAGE)
#define RIGHT(v)              VAL_EQUALS_POS(v, RIGHT_VOLTAGE)
#define DOWN(v)               VAL_EQUALS_POS(v, DOWN_VOLTAGE)
#define LEFT(v)               VAL_EQUALS_POS(v, LEFT_VOLTAGE)
#define BUTTON_PRESSED(v)     VAL_EQUALS_POS(v, BUTTON_VOLTAGE)

/* Convert ADC value to voltage (mV) */
#define ADC_TO_MV(x)          ((uint32_t)x * ADC_REF_VOLTAGE / ADC_MAX_VALUE)

/* Update interval */
#define JOYSTICK_READ_INTERVAL    (CLOCK_SECOND / 100)


static struct adc_module adc_instance;
static uint8_t joystick_state;
static bool sensor_active;
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
    TRACE("new joystick state: %d", joystick_state);
    sensors_changed(&joystick_sensor);
  }

  return true;
}
/*---------------------------------------------------------------------------*/
static void
configure_adc(void)
{
  /* Initialize ADC using PIN_PB02 with 1500 mV reference */
  struct adc_config config_adc;
  adc_get_config_defaults(&config_adc);
  config_adc.positive_input = ADC_POSITIVE_INPUT_PIN10;
  config_adc.reference = ADC_REFERENCE_INTVCC1;
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
  (void)type;

  /* Returns last known state */
  return joystick_state;
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
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(joystick_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

  etimer_set(&et, JOYSTICK_READ_INTERVAL);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    update();

    etimer_restart(&et);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(joystick_sensor, "Joystick Sensor", value, configure, status);
