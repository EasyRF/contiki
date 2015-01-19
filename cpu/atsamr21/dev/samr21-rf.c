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

/* at86rf233 includes */
#include "trx_access.h"
#include "at86rf233.h"

/* Contiki includes */
#include "contiki.h"
#include "netstack.h"
#include "packetbuf.h"
#include "net/mac/frame802154.h"
#include "samr21-rf.h"
#include "lib/sensors.h"
#include "dev/sensor_joystick.h"
#include "dev/leds.h"
#include "log.h"
#include "random.h"

/* Enables RF test code */
//#define MEASURE_RF_CLOCK
//#define TOM_ENABLED
#define PRINT_RF_STATS
//#define CRYSTAL_ADJUST

/* Disable TRACE logging */
#undef TRACE
#define TRACE(...)

/*---------------------------------------------------------------------------*/

#define RSSI_BASE_VAL                 -94
#define PHY_CRC_SIZE                  2

#define SAMR21_MAX_TX_POWER_REG_VAL   0x0f
static const int tx_power_dbm[SAMR21_MAX_TX_POWER_REG_VAL + 1] = {
  4, 3.7, 3.4, 3, 2.5, 2, 1, 0, -1, -2, -3, -4, -6, -8, -12, -17
};

#define OUTPUT_POWER_MIN  tx_power_dbm[SAMR21_MAX_TX_POWER_REG_VAL]
#define OUTPUT_POWER_MAX  tx_power_dbm[0]

#define ON_BOARD_ANTENNA  0x01
#define EXTERNAL_ANTENNA  0x02

/* Use defines above and logic OR to enabled antenna diversity */
#define SELECTED_ANTENNA  (ON_BOARD_ANTENNA)

#define RX_BUFFER_CNT     8

/*---------------------------------------------------------------------------*/

typedef enum {
  PHY_STATE_INITIAL,
  PHY_STATE_IDLE,
  PHY_STATE_SLEEP,
  PHY_STATE_TX_WAIT_END,
} PhyState_t;

/*---------------------------------------------------------------------------*/

/* RF statistics */
#define EMPTY_RF_STATS {0,0,0,0,0,0,0,0,0}
typedef uint32_t rfstat_t;
struct rf_stats_t {
  rfstat_t rx_cnt;
  rfstat_t rx_err_underflow;
  rfstat_t rx_err_crc;
  rfstat_t rx_err_len;
  rfstat_t tx_cnt;
  rfstat_t tx_err_radio;
  rfstat_t tx_err_noack;
  rfstat_t tx_err_collision;
  rfstat_t tx_err_timeout;
};
static volatile struct rf_stats_t rf_stats = EMPTY_RF_STATS;

/* Common */
static uint16_t rnd_seed;
static volatile PhyState_t phyState = PHY_STATE_INITIAL;

/* Tx related */
static uint8_t tx_buffer[PACKETBUF_SIZE];
static volatile uint8_t trac_status;

/* RX related */
static bool phyRxState;
static volatile uint8_t rx_buffer_write_index;
static volatile uint8_t rx_buffer_read_index;
static uint8_t rx_buffer[RX_BUFFER_CNT][PACKETBUF_SIZE + 4];
static volatile bool receiving;

#ifdef TOM_ENABLED
struct TOM_DATA {
  uint32_t tim : 24;
  int8_t   fec;
  uint8_t  cpm[9];
  bool     valid;
} __attribute__ ((packed));
struct TOM_DATA rx_tom;
#endif

/* Crystal capacitance trim value */
static int8_t crystal_cap_trim_value = SAMR21_RF_CRYSTAL_CAP_TRIM_DEFAULT;

/*---------------------------------------------------------------------------*/

static void samr21_interrupt_handler(void);

/*---------------------------------------------------------------------------*/
PROCESS(samr21_rf_process, "SAMR21 RF driver");
/*---------------------------------------------------------------------------*/
static void
gpio_init(void)
{
  struct port_config pin_conf;

  /* Configure output pins RST and SLP and set level high */
  port_get_config_defaults(&pin_conf);
  pin_conf.direction = PORT_PIN_DIR_OUTPUT;
  port_pin_set_config(AT86RFX_RST_PIN, &pin_conf);
  port_pin_set_config(AT86RFX_SLP_PIN, &pin_conf);
  port_pin_set_output_level(AT86RFX_RST_PIN, true);
  port_pin_set_output_level(AT86RFX_SLP_PIN, true);

  /* Enable APB Clock for RFCTRL */
  PM->APBCMASK.reg |= (1<<PM_APBCMASK_RFCTRL_Pos);

  /* Setup RF control register for controlling antenna selection */
  REG_RFCTRL_FECFG = RFCTRL_CFG_ANT_DIV;
  struct system_pinmux_config config_pinmux;
  system_pinmux_get_config_defaults(&config_pinmux);
  config_pinmux.mux_position = MUX_PA09F_RFCTRL_FECTRL1;
  config_pinmux.direction    = SYSTEM_PINMUX_PIN_DIR_OUTPUT;
  system_pinmux_pin_set_config(PIN_RFCTRL1, &config_pinmux);
  system_pinmux_pin_set_config(PIN_RFCTRL2, &config_pinmux);
}
/*---------------------------------------------------------------------------*/
static void
phyTrxSetState(uint8_t state)
{
  /* Keep writing the wanted state until the status register reflects it */
  do {
    trx_reg_write(TRX_STATE_REG, state);
  } while (state != (trx_reg_read(TRX_STATUS_REG) & TRX_STATUS_MASK));
}
/*---------------------------------------------------------------------------*/
unsigned short
samr21_random_rand(void)
{
  return rnd_seed;
}
/*---------------------------------------------------------------------------*/
static int
receiving_packet(void)
{
  return receiving;
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{
  /* Check if a packet is in one of the rx_buffers */
  return rx_buffer_read_index != rx_buffer_write_index;
}
/*---------------------------------------------------------------------------*/
static int
prepare(const void *payload, unsigned short payload_len)
{
  /* Set payload length in first byte of tx_buffer and add 2 bytes for CRC */
  tx_buffer[0] = payload_len + 2;

  /* Copy payload to tx_buffer */
  memcpy(&tx_buffer[1], payload, payload_len);

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
transmit(unsigned short transmit_len)
{
  int tx_result;

  /* Wait until receiving packets is finished */
  while (receiving_packet()) {
    TRACE("receiving packet");
  }

  /* Go to TX ARET state */
  phyTrxSetState(TRX_CMD_TX_ARET_ON);

  /* Clear interrupts */
  trx_reg_read(IRQ_STATUS_REG);

  /* Write payload to frame buffer.
   * Size of the buffer is sent as first byte of the data
   */
  trx_frame_write(tx_buffer, (tx_buffer[0] - 1) /* length value*/);

  /* Set PHY state such that we're waiting for TX END interrupt */
  phyState = PHY_STATE_TX_WAIT_END;

  /* Timestamp before transmission */
  rtimer_clock_t start = rtimer_arch_now();

  /* Send the packet by toggling the SLP line */
  TRX_SLP_TR_HIGH();
  TRX_TRIG_DELAY();
  TRX_SLP_TR_LOW();

  /* Update counters */
  ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
  ENERGEST_ON(ENERGEST_TYPE_TRANSMIT);

  /* Wait for tx result */
  RTIMER_BUSYWAIT_UNTIL((phyState != PHY_STATE_TX_WAIT_END), RTIMER_SECOND);

  /* Timestamp after transmission */
  rtimer_clock_t duration = rtimer_arch_now() - start;
  (void) duration;

  /* If the radio was on, turn it on again */
  if (phyRxState) {
    phyTrxSetState(TRX_CMD_RX_AACK_ON);

    /* Update counters */
    ENERGEST_ON(ENERGEST_TYPE_LISTEN);
  }

  if (phyState == PHY_STATE_TX_WAIT_END) {
    rf_stats.tx_err_timeout++;
    WARN("TX timeout");
    tx_result = RADIO_TX_ERR;
  } else {
    if (trac_status == TRAC_STATUS_SUCCESS) {
      rf_stats.tx_cnt++;
      TRACE("RADIO_TX_OK");
      tx_result = RADIO_TX_OK;
    } else if (trac_status == TRAC_STATUS_NO_ACK) {
      rf_stats.tx_err_noack++;
      WARN("RADIO_TX_NOACK, packet length: %d", transmit_len);
      tx_result = RADIO_TX_NOACK;
    } else if (trac_status == TRAC_STATUS_CHANNEL_ACCESS_FAILURE) {
      rf_stats.tx_err_collision++;
      WARN("RADIO_TX_COLLISION");
      tx_result = RADIO_TX_COLLISION;
    } else {
      rf_stats.tx_err_radio++;
      WARN("RADIO_TX_ERR");
      tx_result = RADIO_TX_ERR;
    }
  }

  TRACE("transmit result: %d, length: %d, time [usec]: %ld",
        tx_result, transmit_len, duration * 1000000 / RTIMER_ARCH_SECOND);

  return tx_result;
}
/*---------------------------------------------------------------------------*/
static int
send(const void *payload, unsigned short payload_len)
{
  /* Just call prepare and transmit */
  prepare(payload, payload_len);
  return transmit(payload_len);
}
/*---------------------------------------------------------------------------*/
static int
read(void *buf, unsigned short buf_len)
{
  /* Check if any buffer is in use */
  if (!pending_packet()) {
    /* No pending packet */
    return 0;
  }

  /* Save the number of received bytes */
  uint8_t len = rx_buffer[rx_buffer_read_index][0];

  /* Check if we actually received data */
  if (len > PHY_CRC_SIZE) {
    /* Read LQI (Package Error Rate: 0=100% 50=97% 100=72% 128=50% 150=25% 200=3% 255=PER 0%) */
    uint8_t lqi = rx_buffer[rx_buffer_read_index][len + 1];

    /* Read and convert ED level to obtain RSSI measurement result is between -94 and -11 dB (accuracy +-5dB)  */
    int8_t rssi = rx_buffer[rx_buffer_read_index][len + 2] + RSSI_BASE_VAL;

    /* Log */
#ifdef TOM_ENABLED
    TRACE("received: %d bytes, rssi: %d, lqi: %d, TOM %d (FEC %d TIM %ld)",
          len, rssi, lqi, rx_tom.valid, rx_tom.fec, (long)rx_tom.tim);
#else
    TRACE("received: %d bytes, rssi: %d, lqi: %d", len, rssi, lqi);
#endif

    rf_stats.rx_cnt++;

    /* Store the length of the packet */
    packetbuf_set_datalen(len - PHY_CRC_SIZE);

    /* Store data of the packet buffer */
    memcpy(buf, rx_buffer[rx_buffer_read_index] + 1, len - PHY_CRC_SIZE);

    /* Store RSSI in dBm for the received packet */
    packetbuf_set_attr(PACKETBUF_ATTR_RSSI, rssi);

    /* Store link quality indicator */
    packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, lqi);
  }

  /* Increase read index */
  rx_buffer_read_index = (rx_buffer_read_index + 1) % RX_BUFFER_CNT;

  /* Check if there are more packets */
  if (pending_packet()) {
    /* Poll the process */
    process_poll(&samr21_rf_process);
  }

  /* Return the number of received bytes */
  return len;
}
/*---------------------------------------------------------------------------*/
static int
channel_clear(void)
{
  /* CCA is done by HW, assume channel is always clear */
  return SAMR21_RF_CCA_CLEAR;
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  /* Turn of radio */
  phyTrxSetState(TRX_CMD_RX_AACK_ON);

  /* Update rx flag */
  phyRxState = true;

  /* Update counters */
  ENERGEST_ON(ENERGEST_TYPE_LISTEN);

  /* Log */
  TRACE("on");

  /* Ok */
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  /* Turn off radio */
  phyTrxSetState(TRX_CMD_TRX_OFF);

  /* Update rx flag */
  phyRxState = false;

  /* Update counters */
  ENERGEST_OFF(ENERGEST_TYPE_LISTEN);

  /* Log */
  TRACE("off");

  /* Ok */
  return 0;
}
/*---------------------------------------------------------------------------*/
static uint8_t
get_channel(void)
{
  /* Return current channel value from transceiver */
  return (trx_reg_read(PHY_CC_CCA_REG) & SAMR21_CCA_CHANNEL_MASK);
}
/*---------------------------------------------------------------------------*/
static void
set_channel(uint8_t channel)
{
  uint8_t reg;

  /* Get current register value */
  reg = trx_reg_read(PHY_CC_CCA_REG) & ~0x1f;
  /* Update channel and write register */
  trx_reg_write(PHY_CC_CCA_REG, reg | channel);
}
/*---------------------------------------------------------------------------*/
static uint16_t
get_pan_id(void)
{
  uint16_t pan_id;
  uint8_t *d = (uint8_t *)&pan_id;

  /* Read PAN ID from transceiver */
  d[0] = trx_reg_read(PAN_ID_0_REG);
  d[1] = trx_reg_read(PAN_ID_1_REG);

  return pan_id;
}
/*---------------------------------------------------------------------------*/
static void
set_pan_id(uint16_t panId)
{
  uint8_t *d = (uint8_t *)&panId;

  /* Write PAN ID to transceiver */
  trx_reg_write(PAN_ID_0_REG, d[0]);
  trx_reg_write(PAN_ID_1_REG, d[1]);
}
/*---------------------------------------------------------------------------*/
static uint16_t
get_short_addr(void)
{
  uint16_t short_id;
  uint8_t *d = (uint8_t *)&short_id;

  /* Read short address from transceiver */
  d[0] = trx_reg_read(SHORT_ADDR_0_REG);
  d[1] = trx_reg_read(SHORT_ADDR_1_REG);

  return short_id;
}
/*---------------------------------------------------------------------------*/
static void
set_short_addr(uint16_t addr)
{
  uint8_t *d = (uint8_t *)&addr;

  /* Write short address to transceiver */
  trx_reg_write(SHORT_ADDR_0_REG, d[0]);
  trx_reg_write(SHORT_ADDR_1_REG, d[1]);
}
/*---------------------------------------------------------------------------*/
/* Returns the current TX power in dBm */
static radio_value_t
get_tx_power(void)
{
  /* Read tx power register (last 4 bits) */
  uint8_t tmp = trx_reg_read(PHY_TX_PWR_REG) & 0x0f;
  /* Check value */
  if (tmp > SAMR21_MAX_TX_POWER_REG_VAL) {
    WARN("Invalid power mode, returning max value");
    return tx_power_dbm[0];
  } else {
    /* Use value as index to convert raw value to dBm */
    return tx_power_dbm[tmp];
  }
}
/*---------------------------------------------------------------------------*/
static void
set_tx_power(radio_value_t requested_tx_power)
{
  uint8_t i, pwr;

  /* Iterate the power modes in increasing order,
   * stop when power exceeds the requested value
   */
  for (i = SAMR21_MAX_TX_POWER_REG_VAL; i >= 0; i--) {
    if (tx_power_dbm[i] > requested_tx_power) {
      if (i != SAMR21_MAX_TX_POWER_REG_VAL) {
        i++; /* One power level back */
      }
      break;
    }
  }

  /* Read current register value */
  pwr = trx_reg_read(PHY_TX_PWR_REG) & ~0x0f;

  /* Write updated value */
  trx_reg_write(PHY_TX_PWR_REG, pwr | i);
}
/*---------------------------------------------------------------------------*/
/* Returns the current CCA threshold in dBm */
/* P_THRES[dBm] = RSSI_BASE_VAL[dBm] + 2[dB] x CCA_ED_THRES */
static radio_value_t
get_cca_threshold(void)
{
  /* Read CCA threshold value from register */
  uint8_t cca_ed_thres = trx_reg_read(CCA_THRES_REG) & 0x0f;
  /* Convert raw value to dBm */
  return RSSI_BASE_VAL + 2 * cca_ed_thres;
}
/*---------------------------------------------------------------------------*/
/* Set the current CCA threshold in dBm */
/* CCA_ED_THRES = (P_THRES[dBm] - RSSI_BASE_VAL[dBm]) / 2 */
static void
set_cca_threshold(radio_value_t value)
{
  uint8_t reg;
  /* Convert dBm value to raw value */
  uint8_t cca_ed_thres = (value - RSSI_BASE_VAL) / 2;
  /* Read current register value */
  reg = trx_reg_read(PHY_TX_PWR_REG) & ~0x0f;
  /* Write updated value */
  trx_reg_write(CCA_THRES_REG, reg | cca_ed_thres);
}
/*---------------------------------------------------------------------------*/
static int8_t
get_energy_level(void)
{
  uint8_t ed;

  /* Turn on radio when necessary */
  if (!phyRxState) {
    phyTrxSetState(TRX_CMD_RX_ON);

    /* Update counters */
    ENERGEST_ON(ENERGEST_TYPE_LISTEN);
  }

  /* Write some value to ED register to start energy detection */
  trx_reg_write(PHY_ED_LEVEL_REG, 0);

  /* Wait for the status register to reflects ED done */
  while (0 == (trx_reg_read(IRQ_STATUS_REG) & (1 << CCA_ED_DONE))) {}

  /* Read the measured energy */
  ed = (int8_t)trx_reg_read(PHY_ED_LEVEL_REG);

  /* Turn off radio when it was off */
  if (!phyRxState) {
    phyTrxSetState(TRX_CMD_TRX_OFF);

    /* Update counters */
    ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
  }

  /* Convert raw value to dBm and return it */
  return ed + RSSI_BASE_VAL;
}
/*---------------------------------------------------------------------------*/
static void
set_crystal_cap_trim(radio_value_t value)
{
  crystal_cap_trim_value = (int8_t)value;

  bool rx_was_on = phyRxState;

  if (rx_was_on) {
    off();
  }

  trx_reg_write(XOSC_CTRL_REG, (0xF << XTAL_MODE) | (crystal_cap_trim_value << XTAL_TRIM));

  if (rx_was_on) {
    on();
  }

}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_value(radio_param_t param, radio_value_t *value)
{
  /* Check output variable pointer */
  if(!value) {
    return RADIO_RESULT_INVALID_VALUE;
  }

  /* Return value depending on requested parameter */
  switch(param) {
  case RADIO_PARAM_POWER_MODE:
    *value = phyRxState ? RADIO_POWER_MODE_ON : RADIO_POWER_MODE_OFF;
    return RADIO_RESULT_OK;
  case RADIO_PARAM_CHANNEL:
    *value = (radio_value_t)get_channel();
    return RADIO_RESULT_OK;
  case RADIO_PARAM_PAN_ID:
    *value = get_pan_id();
    return RADIO_RESULT_OK;
  case RADIO_PARAM_16BIT_ADDR:
    *value = get_short_addr();
    return RADIO_RESULT_OK;
  case RADIO_PARAM_RX_MODE:
    *value = 0;
    if((trx_reg_read(TRX_STATUS_REG) & TRX_STATUS_MASK) == TRX_STATUS_RX_AACK_ON) {
      *value |= RADIO_RX_MODE_AUTOACK;
      *value |= RADIO_RX_MODE_ADDRESS_FILTER;
    }
    return RADIO_RESULT_OK;
  case RADIO_PARAM_TXPOWER:
    *value = get_tx_power();
    return RADIO_RESULT_OK;
  case RADIO_PARAM_CCA_THRESHOLD:
    *value = get_cca_threshold();
    return RADIO_RESULT_OK;
  case RADIO_PARAM_RSSI:
    TRACE("RADIO_PARAM_RSSI");
    *value = get_energy_level();
    return RADIO_RESULT_OK;
  case RADIO_PARAM_CRYSTAL_CAP_TRIM:
    *value = crystal_cap_trim_value;
    return RADIO_RESULT_OK;
  case RADIO_CONST_CHANNEL_MIN:
    *value = SAMR21_RF_CHANNEL_MIN;
    return RADIO_RESULT_OK;
  case RADIO_CONST_CHANNEL_MAX:
    *value = SAMR21_RF_CHANNEL_MAX;
    return RADIO_RESULT_OK;
  case RADIO_CONST_TXPOWER_MIN:
    *value = OUTPUT_POWER_MIN;
    return RADIO_RESULT_OK;
  case RADIO_CONST_TXPOWER_MAX:
    *value = OUTPUT_POWER_MAX;
    return RADIO_RESULT_OK;
  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_value(radio_param_t param, radio_value_t value)
{
  /* Set value depending on the given parameter */
  switch(param) {
  case RADIO_PARAM_POWER_MODE:
    if(value == RADIO_POWER_MODE_ON) {
      on();
      return RADIO_RESULT_OK;
    }
    if(value == RADIO_POWER_MODE_OFF) {
      off();
      return RADIO_RESULT_OK;
    }
    return RADIO_RESULT_INVALID_VALUE;
  case RADIO_PARAM_CHANNEL:
    if(value < SAMR21_RF_CHANNEL_MIN ||
       value > SAMR21_RF_CHANNEL_MAX) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    set_channel(value);
    return RADIO_RESULT_OK;
  case RADIO_PARAM_PAN_ID:
    set_pan_id(value & 0xffff);
    return RADIO_RESULT_OK;
  case RADIO_PARAM_16BIT_ADDR:
    set_short_addr(value & 0xffff);
    return RADIO_RESULT_OK;
  case RADIO_PARAM_RX_MODE:
    if(value & ~(RADIO_RX_MODE_ADDRESS_FILTER |
                 RADIO_RX_MODE_AUTOACK)) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    /* TODO: Also support basic mode, without auto ack and frame filtering */

    return RADIO_RESULT_OK;
  case RADIO_PARAM_TXPOWER:
    if(value < OUTPUT_POWER_MIN || value > OUTPUT_POWER_MAX) {
      WARN("Invalid TX power value");
      return RADIO_RESULT_INVALID_VALUE;
    }

    set_tx_power(value);
    return RADIO_RESULT_OK;
  case RADIO_PARAM_CCA_THRESHOLD:
    set_cca_threshold(value);
    return RADIO_RESULT_OK;
  case RADIO_PARAM_CRYSTAL_CAP_TRIM:
    if (value < SAMR21_RF_CRYSTAL_CAP_TRIM_MIN || value > SAMR21_RF_CRYSTAL_CAP_TRIM_MAX) {
      WARN("Invalid crystal cap value");
      return RADIO_RESULT_INVALID_VALUE;
    }
    set_crystal_cap_trim(value);
    return RADIO_RESULT_OK;
  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_object(radio_param_t param, void *dest, size_t size)
{
  uint8_t *target;
  int i;

  /* We only have the 64-bit long address as object */
  if(param == RADIO_PARAM_64BIT_ADDR) {
    /* Length must be 8 bytes */
    if(size != 8 || !dest) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    target = dest;
    for(i = 0; i < 8; i++) {
      target[7 - i] = trx_reg_read(IEEE_ADDR_0_REG + i);
    }

    return RADIO_RESULT_OK;
  }
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_object(radio_param_t param, const void *src, size_t size)
{
  int i;

  /* We only have the 64-bit long address as object */
  if(param == RADIO_PARAM_64BIT_ADDR) {
    /* Length must be 8 bytes */
    if(size != 8 || !src) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    for(i = 0; i < 8; i++) {
      trx_reg_write(IEEE_ADDR_0_REG + i, ((uint8_t *)src)[7 - i]);
    }

    return RADIO_RESULT_OK;
  }
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static void
samr21_interrupt_handler(void)
{
  uint8_t size;

  /* Update counters */
  ENERGEST_ON(ENERGEST_TYPE_IRQ);

  /* When asleep we don't handle interrupts */
  if (PHY_STATE_SLEEP == phyState) {
    goto end;
  }

  uint8_t int_status = trx_reg_read(IRQ_STATUS_REG);

  if (int_status & (1 << RX_START)) {
    receiving = true;
  }

  /* Only handle TRX_END interrupt */
  if (int_status & (1 << TRX_END)) {
    /* The interrupt is either caused by a TX or RX
     * When PHY state is IDLE it is RX
     */
    if (PHY_STATE_IDLE == phyState) {

      receiving = false;

      TRACE("rx_buffer_read_index: %d, rx_buffer_write_index: %d", rx_buffer_read_index, rx_buffer_write_index);

      /* Check if all rx buffers are in use */
      if (rx_buffer_write_index == ((rx_buffer_read_index + RX_BUFFER_CNT - 1) % RX_BUFFER_CNT)) {
        rf_stats.rx_err_underflow++;
        WARN("RF underflow");
        goto end;
      }

      /* Read first byte from frame buffer which holds the PSDU size (data + 2 bytes CRC) */
      trx_frame_read(&size, 1);

      if (size > 127) {
        rf_stats.rx_err_len++;
        WARN("RF length error");
        goto end;
      }

      /* Read again from the frame buffer and this time read size + 3 bytes (length byte + (data + CRC) + LQI + ED)
       * Size is read again and LQI is included which is stored in the last byte */
      trx_frame_read(rx_buffer[rx_buffer_write_index], size + 3);

#ifdef TOM_ENABLED
      /* Get TOM data. after 114 bytes of data the TOM data is corrupted by overlapping frame RAM space */
      rx_tom.valid = size < 114;

      /* Read TOM data from SRAM address 0x73 */
      if (rx_tom.valid) {
        trx_sram_read(0x7C, (uint8_t *)&rx_tom.fec, 1);
      }
#endif
      /* Poll the process */
      process_poll(&samr21_rf_process);

      /* Increase write index */
      rx_buffer_write_index = (rx_buffer_write_index + 1) % RX_BUFFER_CNT;

    } else if (PHY_STATE_TX_WAIT_END == phyState) {
      /* Interrupt was caused by a TX, read TX result */
      trac_status = (trx_reg_read(TRX_STATE_REG) >> TRAC_STATUS) & 7;

      /* Update state */
      phyState = PHY_STATE_IDLE;
    }
  }

end:

  /* Update counters */
  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}
/*---------------------------------------------------------------------------*/
static uint16_t
get_rnd_value(void)
{
  uint16_t rnd = 0;
  uint8_t rndValue;

  bool rx_was_on = phyRxState;

  if (!rx_was_on) {
    on();
  }

  for (uint8_t i = 0; i < 16; i += 2) {
    delay_us(RANDOM_NUMBER_UPDATE_INTERVAL);
    rndValue = (trx_reg_read(PHY_RSSI_REG) >> RND_VALUE) & 3;
    rnd |= rndValue << i;
  }

  if (!rx_was_on) {
    off();
  }

  TRACE("returning rnd value %d", rnd);

  return rnd;
}
/*---------------------------------------------------------------------------*/
#ifdef MEASURE_RF_CLOCK
#include <gclk.h>

#define PWM_MODULE    TC3
#define PWM_OUT_PIN   PIN_PA15E_TC3_WO1
#define PWM_OUT_MUX   MUX_PA15E_TC3_WO1

static struct tc_module tc_instance;

static void
configure_tc(void)
{
  leds_off(LEDS_RED);

  /* Show revision of RF chip since the GCLK_IO differs for rev. A */
  INFO("DID: %lX", REG_DSU_DID);

  struct system_pinmux_config mux_conf;
  system_pinmux_get_config_defaults(&mux_conf);
  mux_conf.direction = SYSTEM_PINMUX_PIN_DIR_INPUT;
  mux_conf.input_pull = SYSTEM_PINMUX_PIN_PULL_NONE;
  mux_conf.mux_position = 5;
  system_pinmux_pin_set_config(PIN_PC16, &mux_conf);

  struct system_gclk_gen_config gclk_config;
  gclk_config.source_clock = SYSTEM_CLOCK_SOURCE_GCLKIN;
  gclk_config.high_when_disabled = false;
  gclk_config.division_factor = 1;
  gclk_config.run_in_standby = false;
  gclk_config.output_enable = false;
  system_gclk_gen_set_config(GCLK_GENERATOR_5, &gclk_config);
  system_gclk_gen_enable(GCLK_GENERATOR_5);

  struct tc_config config_tc;
  tc_get_config_defaults(&config_tc);
  config_tc.clock_source = GCLK_GENERATOR_5;
  config_tc.counter_size = TC_COUNTER_SIZE_16BIT;
  config_tc.wave_generation = TC_WAVE_GENERATION_NORMAL_PWM;
  config_tc.counter_16_bit.compare_capture_channel[1] = 0xFFFF - (0xFFFF / 100);
  config_tc.pwm_channel[1].enabled = true;
  config_tc.pwm_channel[1].pin_out = PWM_OUT_PIN;
  config_tc.pwm_channel[1].pin_mux = PWM_OUT_MUX;
  tc_init(&tc_instance, PWM_MODULE, &config_tc);
  tc_enable(&tc_instance);
}
#endif
/*---------------------------------------------------------------------------*/
static int
init(void)
{
  INFO("Initializing at86rf233 transceiver");

  /* Initalize GPIO pins */
  gpio_init();

  /* Initialize SPI peripheral */
  trx_spi_init();

  /* Reset the transceiver by toggling RST line */
  PhyReset();

  /* Radio is initially off */
  phyRxState = false;

  /* Transceiver state is initially idle */
  phyState = PHY_STATE_IDLE;

  /* Turn radio off */
  phyTrxSetState(TRX_CMD_TRX_OFF);

  /* 8 MHz to CPU */
  trx_reg_write(TRX_CTRL_0_REG, 0x4);// | (1 << TOM_EN) | (1 << PMU_EN));

  /* Auto generate CRC, Enable monitor IRQ status, always show interrupt in status register */
  trx_reg_write(TRX_CTRL_1_REG, (1 << TX_AUTO_CRC_ON) | (3 << SPI_CMD_MODE) | (1 << IRQ_MASK_MODE));

  /* Enable frame buffer protection and keep OQPSK_SCRAM_EN bit on its reset value */
  trx_reg_write(TRX_CTRL_2_REG, (1 << RX_SAFE_MODE) | (1 << OQPSK_SCRAM_EN));

#if (SELECTED_ANTENNA == (ON_BOARD_ANTENNA + EXTERNAL_ANTENNA))
  /* Use the antenna diversity algorithm */
  trx_reg_write(ANT_DIV_REG, (1 << ANT_EXT_SW_EN) | (1 << ANT_DIV_EN));
#else
  /* Use specific antenna */
  trx_reg_write(ANT_DIV_REG, (1 << ANT_EXT_SW_EN) | SELECTED_ANTENNA);
#endif

  /* Enable antenna switch and select specific antenna */

  /* Set crystal capacitance value */
  set_crystal_cap_trim(SAMR21_RF_CRYSTAL_CAP_TRIM_DEFAULT);

  /* Install transceiver interrupt handler */
  trx_irq_init(samr21_interrupt_handler);

  /* Enable TRX_END interrupt */
  trx_reg_write(IRQ_MASK_REG, (1 << TRX_END) | (1 << RX_START));

  /* Set random SEED for CSMA-CA */
  rnd_seed = get_rnd_value();

  INFO("CCA random seed: %d", rnd_seed);

  /* Use random value as CSMA seed */
  trx_reg_write(CSMA_SEED_0_REG, rnd_seed & 0xff);
  trx_reg_write(CSMA_SEED_1_REG, trx_reg_read(CSMA_SEED_1_REG) | ((rnd_seed >> 8) & 0x07));

  /* Start a process for handling incoming RF packets */
  process_start(&samr21_rf_process, NULL);

  /* Log */
  INFO("at86rf233 initialized");

#ifdef MEASURE_RF_CLOCK
  /* Code for measuring RF frequency */
  configure_tc();
#endif

  return 0;
}
/*---------------------------------------------------------------------------*/
#ifdef CRYSTAL_ADJUST
PROCESS(crystal_cap_adjust_process, "crystal_cap_adjust_process");
PROCESS_THREAD(crystal_cap_adjust_process, ev, data)
{
  struct sensors_sensor *sensor;

  PROCESS_BEGIN();

  process_start(&sensors_process, NULL);
  SENSORS_ACTIVATE(joystick_sensor);

  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event);

    sensor = (struct sensors_sensor *)data;

    if (sensor == &joystick_sensor ) {
      if (joystick_sensor.value(JOYSTICK_STATE) == JOYSTICK_UP ||
          joystick_sensor.value(JOYSTICK_STATE) == JOYSTICK_DOWN) {
        int cap_trim;
        NETSTACK_RADIO.get_value(RADIO_PARAM_CRYSTAL_CAP_TRIM, &cap_trim);

        if (joystick_sensor.value(JOYSTICK_STATE) == JOYSTICK_UP) {
          cap_trim += 1;
        } else if (joystick_sensor.value(JOYSTICK_STATE) == JOYSTICK_DOWN) {
          cap_trim -= 1;
        }

        /* Adjust crystal cap trim */
        if (NETSTACK_RADIO.set_value(RADIO_PARAM_CRYSTAL_CAP_TRIM, cap_trim) == RADIO_RESULT_OK) {
          INFO("New cap trim = %d", cap_trim);
        }
      }
    }
  }

  PROCESS_END();
}
#endif
/*---------------------------------------------------------------------------*/
#ifdef PRINT_RF_STATS
/*---------------------------------------------------------------------------*/
static void
print_rf_stats(bool force_print)
{
  static struct rf_stats_t last_stats = EMPTY_RF_STATS;

  if (memcmp((void *)&last_stats, (void *)&rf_stats, sizeof(rf_stats)) != 0) {
    force_print = true;
  }

  memcpy((void *)&last_stats, (void *)&rf_stats, sizeof(rf_stats));

  if (force_print) {
    INFO("******** RECEIVE STATISTICS ***********");
    INFO("* PASSED        : %ld" , rf_stats.rx_cnt);
    INFO("* ERR crc       : %ld" , rf_stats.rx_err_crc);
    INFO("* ERR len       : %ld" , rf_stats.rx_err_len);
    INFO("* ERR underflow : %ld" , rf_stats.rx_err_underflow);
#ifdef TOM_ENABLED
    INFO("* TOM.FEC       : %d"  , rx_tom.fec);
#endif
    INFO("******** TRANSMIT STATISTICS **********");
    INFO("* PASSED        : %ld" , rf_stats.tx_cnt);
    INFO("* ERR collision : %ld" , rf_stats.tx_err_collision);
    INFO("* ERR no ack    : %ld" , rf_stats.tx_err_noack);
    INFO("* ERR radio     : %ld" , rf_stats.tx_err_radio);
    INFO("* ERR timeout   : %ld" , rf_stats.tx_err_timeout);
    INFO("***************************************");
  }
}
/*---------------------------------------------------------------------------*/
PROCESS(printf_rf_stats_process, "Print RF Stats");
PROCESS_THREAD(printf_rf_stats_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

  while(1) {
    etimer_set(&et, CLOCK_SECOND * 5);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    print_rf_stats(true);
  }

  PROCESS_END();
}
#endif
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(samr21_rf_process, ev, data)
{
  PROCESS_BEGIN();

  TRACE("samr21_rf_process started");

#ifdef PRINT_RF_STATS
  process_start(&printf_rf_stats_process, 0);
#endif
#ifdef CRYSTAL_ADJUST
  process_start(&crystal_cap_adjust_process, 0);
#endif

  while(1) {
    /* Block until process is polled from the interrupt handler */
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

    /* Clear global packet buffer */
    packetbuf_clear();

    /* Fill packet buffer with received data and its attributes */
    /* Be sure a packet has been received by checking the return value */
    if (read(packetbuf_dataptr(), PACKETBUF_SIZE) > 0) {
      /* Let the network stack process the packet */
      NETSTACK_RDC.input();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
const struct radio_driver samr21_rf_driver =
{
  init,
  prepare,
  transmit,
  send,
  read,
  channel_clear,
  receiving_packet,
  pending_packet,
  on,
  off,
  get_value,
  set_value,
  get_object,
  set_object
};
