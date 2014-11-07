#include <asf.h>
#include "contiki.h"
#include "log.h"
#include "dev/leds.h"
#include "sensor_qtouch_wheel.h"


/* Enable peripheral */
#define PTC_APBC_BITMASK  (1u << 19u)

/* Calculate measure frequncy */
#define TOUCH_FREQUENCY   (1000 / DEF_TOUCH_MEASUREMENT_PERIOD_MS)

/*----------------------------------------------------------------------------
*                           manifest constants
*  ----------------------------------------------------------------------------*/
#define DEF_SELFCAP_CAL_SEQ1_COUNT          8
#define DEF_SELFCAP_CAL_SEQ2_COUNT          4
#define DEF_SELFCAP_CC_CAL_CLK_PRESCALE     PRSC_DIV_SEL_8
#define DEF_SELFCAP_CC_CAL_SENSE_RESISTOR   RSEL_VAL_100
#define DEF_SELFCAP_QUICK_REBURST_ENABLE    1u

/* Flag indicating if aquisition is active */
bool sensor_active;

/* ! QTouch Library Timing info. */
touch_time_t touch_time;

/* Self Cap sensors measured data pointer */
touch_measure_data_t *p_selfcap_measure_data = NULL;

/* Self Cap sensors data block provided as input to Touch library */
static uint8_t selfcap_data_blk[PRIV_SELFCAP_DATA_BLK_SIZE];

/* Self Cap sensors Pins Info */
uint16_t selfcap_y_nodes[DEF_SELFCAP_NUM_CHANNELS] = {DEF_SELFCAP_LINES};

/* Self Cap gain per touch channel */
gain_t selfcap_gain_per_node[DEF_SELFCAP_NUM_CHANNELS] = {DEF_SELFCAP_GAIN_PER_NODE};

/* PTC acquisition frequency delay setting */
freq_hop_sel_t selfcap_freq_hops[3u] = {DEF_SELFCAP_HOP_FREQS};


/* Self Cap Configuration structure provided as input to Touch Library. */
static touch_selfcap_config_t selfcap_config = {
  DEF_SELFCAP_NUM_CHANNELS,
  DEF_SELFCAP_NUM_SENSORS,
  DEF_SELFCAP_NUM_ROTORS_SLIDERS,
  {
    DEF_SELFCAP_DI,
    DEF_SELFCAP_ATCH_DRIFT_RATE,
    DEF_SELFCAP_TCH_DRIFT_RATE,
    DEF_SELFCAP_MAX_ON_DURATION,
    DEF_SELFCAP_DRIFT_HOLD_TIME,
    DEF_SELFCAP_ATCH_RECAL_DELAY,
    DEF_SELFCAP_CAL_SEQ1_COUNT,
    DEF_SELFCAP_CAL_SEQ2_COUNT,
    DEF_SELFCAP_ATCH_RECAL_THRESHOLD
  },
  {
    selfcap_gain_per_node,
    DEF_SELFCAP_FREQ_MODE,
    DEF_SELFCAP_CLK_PRESCALE,
    DEF_SELFCAP_SENSE_RESISTOR,
    DEF_SELFCAP_CC_CAL_CLK_PRESCALE,
    DEF_SELFCAP_CC_CAL_SENSE_RESISTOR,
    selfcap_freq_hops,
    DEF_SELFCAP_FILTER_LEVEL,
    DEF_SELFCAP_AUTO_OS,
  },
  selfcap_data_blk,
  PRIV_SELFCAP_DATA_BLK_SIZE,
  selfcap_y_nodes,
  DEF_SELFCAP_QUICK_REBURST_ENABLE,
  DEF_SELFCAP_FILTER_CALLBACK
};

/* Touch Library input configuration structure */
touch_config_t touch_config = {
  NULL,                     /* NO Multi Cap configuration */
  &selfcap_config,          /* Pointer to Self Cap configuration structure. */
  DEF_TOUCH_PTC_ISR_LVL,    /* PTC interrupt level. */
};


static struct tc_module tc_instance;
static volatile uint8_t wheel_state;
static volatile uint8_t wheel_position;

/*---------------------------------------------------------------------------*/
static void
touch_configure_ptc_clock(void)
{
  struct system_gclk_chan_config gclk_chan_conf;

  system_gclk_chan_get_config_defaults(&gclk_chan_conf);

  gclk_chan_conf.source_generator = GCLK_GENERATOR_1;

  system_gclk_chan_set_config(PTC_GCLK_ID, &gclk_chan_conf);

  system_gclk_chan_enable(PTC_GCLK_ID);

  system_apb_clock_set_mask(SYSTEM_CLOCK_APB_APBC, PTC_APBC_BITMASK);

  INFO("PTC enabled");
}
/*---------------------------------------------------------------------------*/
static touch_ret_t
touch_sensors_config(void)
{
  touch_ret_t touch_ret;
  sensor_id_t sensor_id;

  touch_ret = touch_selfcap_sensor_config(
        SENSOR_TYPE_ROTOR, CHANNEL_0,
        CHANNEL_2, NO_AKS_GROUP, 40u,
        HYST_6_25, RES_8_BIT,
        &sensor_id);

  if (touch_ret == TOUCH_SUCCESS) {
    INFO("Sensor %d configured", sensor_id);
  }

  return touch_ret;
}
/*---------------------------------------------------------------------------*/
static void
touch_selfcap_measure_complete_callback( void )
{
  if (!(p_selfcap_measure_data->acq_status & TOUCH_BURST_AGAIN)) {
    p_selfcap_measure_data->measurement_done_touch = 1u;
  }
}
/*---------------------------------------------------------------------------*/
static touch_ret_t
touch_sensor_measure(void)
{
  if (p_selfcap_measure_data->measurement_done_touch == 1) {
    uint8_t new_wheel_state, new_wheel_position;
    new_wheel_state = GET_SELFCAP_SENSOR_STATE(0);
    new_wheel_position = GET_SELFCAP_ROTOR_SLIDER_POSITION(0);

    if (new_wheel_state != wheel_state || new_wheel_position != wheel_position) {
      wheel_state = new_wheel_state;
      wheel_position = new_wheel_position;

      sensors_changed(&touch_wheel_sensor);
    }
  }

  p_selfcap_measure_data->measurement_done_touch = 0;

  return touch_selfcap_sensors_measure(
      touch_time.current_time_ms,
      NORMAL_ACQ_MODE,
      touch_selfcap_measure_complete_callback);
}
/*---------------------------------------------------------------------------*/
static touch_ret_t
qtouch_wheel_init(void)
{
  touch_ret_t touch_ret = TOUCH_SUCCESS;

  /* Setup and enable generic clock source for PTC module. */
  touch_configure_ptc_clock();

  /* Initialize touch library for Self Cap operation. */
  touch_ret = touch_selfcap_sensors_init(&touch_config);
  if (touch_ret != TOUCH_SUCCESS) {
    return touch_ret;
  }

  /* configure the touch library sensor */
  touch_ret = touch_sensors_config();
  if (touch_ret != TOUCH_SUCCESS) {
    return touch_ret;
  }

  /* auto calibrate sensor */
  touch_ret = touch_selfcap_sensors_calibrate(AUTO_TUNE_RSEL);
  if (touch_ret != TOUCH_SUCCESS) {
    return touch_ret;
  }

  touch_time.current_time_ms = 0;
  touch_time.measurement_period_ms = DEF_TOUCH_MEASUREMENT_PERIOD_MS;

  return touch_ret;
}
/*---------------------------------------------------------------------------*/
static void
tc_callback(struct tc_module *const module_inst)
{
  touch_time.time_to_measure_touch = 1;
  touch_time.current_time_ms += touch_time.measurement_period_ms;

  touch_sensor_measure();
}
/*---------------------------------------------------------------------------*/
static void
configure_timer(void)
{
  struct tc_config config_tc;

  tc_get_config_defaults(&config_tc);

  config_tc.counter_size = TC_COUNTER_SIZE_8BIT;
  config_tc.clock_source = GCLK_GENERATOR_1;
  config_tc.clock_prescaler = TC_CLOCK_PRESCALER_DIV1024;
  config_tc.counter_8_bit.period =
      (system_gclk_gen_get_hz(GCLK_GENERATOR_1) / 1024) / TOUCH_FREQUENCY;

  tc_init(&tc_instance, TOUCH_TC_MODULE, &config_tc);

  tc_register_callback(&tc_instance, tc_callback, TC_CALLBACK_OVERFLOW);
  tc_enable_callback(&tc_instance, TC_CALLBACK_OVERFLOW);
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  switch (type) {
  case TOUCH_WHEEL_STATE:
    return wheel_state;
  case TOUCH_WHEEL_POSITION:
    return wheel_position;
  default:
    WARN("Requested unkown value");
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
    qtouch_wheel_init();
    configure_timer();
    return 1;
  case SENSORS_ACTIVE:
    if (value) {
      tc_enable(&tc_instance);
      sensor_active = true;
    } else {
      tc_disable(&tc_instance);
      sensor_active = false;
    }
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(touch_wheel_sensor, "QTouch Wheel Sensor", value, configure, status);
