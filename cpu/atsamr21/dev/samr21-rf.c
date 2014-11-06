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

#include "log.h"

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


#define ON_BOARD_ANTENNA              0x01
#define EXTERNAL_ANTENNA              0x02

/*---------------------------------------------------------------------------*/

typedef enum {
  PHY_STATE_INITIAL,
  PHY_STATE_IDLE,
  PHY_STATE_SLEEP,
  PHY_STATE_TX_WAIT_END,
} PhyState_t;

/*---------------------------------------------------------------------------*/

/* Common */
static uint16_t rnd_seed;
static volatile PhyState_t phyState = PHY_STATE_INITIAL;

/* Tx related */
static uint8_t tx_buffer[PACKETBUF_SIZE];
static volatile uint8_t trac_status;

/* RX related */
static bool phyRxState;
static uint8_t rx_buffer[PACKETBUF_SIZE];
static volatile uint8_t rx_size;
static volatile uint8_t rx_lqi;
static volatile int8_t rx_rssi;

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

  /* Setup RF Control register for controlling antenna selection */
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
  /* Read status register */
  uint8_t status = trx_reg_read(TRX_STATUS_REG) & TRX_STATUS_MASK;
  /* Check if state is in one of the RX busy states */
  int res = (status == TRX_STATUS_BUSY_RX || status == TRX_STATUS_BUSY_RX_AACK);
  if (res) WARN("receiving");
  return res;
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{
  /* Check if a packet is in the rx_buffer */
  int res = (rx_size > 0);
  if (res) WARN("pending");
  return res;
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
  /* Log */
  TRACE("transmitting %d bytes", transmit_len);

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

  /* Send the packet by toggling the SLP line */
  TRX_SLP_TR_HIGH();
  TRX_TRIG_DELAY();
  TRX_SLP_TR_LOW();

  /* Wait for tx result */
  RTIMER_BUSYWAIT_UNTIL((phyState != PHY_STATE_TX_WAIT_END), RTIMER_SECOND);

  if (phyState == PHY_STATE_TX_WAIT_END) {
    WARN("TX timeout");
    return RADIO_TX_ERR;
  }

  if (trac_status == TRAC_STATUS_SUCCESS) {
    TRACE("RADIO_TX_OK");
    return RADIO_TX_OK;
  } else if (trac_status == TRAC_STATUS_NO_ACK) {
    WARN("RADIO_TX_NOACK");
    return RADIO_TX_NOACK;
  } else if (trac_status == TRAC_STATUS_CHANNEL_ACCESS_FAILURE) {
    WARN("RADIO_TX_COLLISION");
    return RADIO_TX_COLLISION;
  } else {
    WARN("RADIO_TX_ERR");
    return RADIO_TX_ERR;
  }
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
  /* Save the number of received bytes */
  uint8_t len = rx_size;

  /* Check if we actually received data */
  if (len > 0) {
    /* Check available buffer space */
    if (buf_len < len) {
      WARN("Packet does not fit buffer");
      return 0;
    }

    /* Log */
    INFO("received: %d bytes, rssi: %d", len, rx_rssi);

    /* Store the length of the packet */
    packetbuf_set_datalen(len);

    /* Store data of the packet buffer */
    memcpy(buf, rx_buffer + 1, len);

    /* Store RSSI in dBm for the received packet */
    packetbuf_set_attr(PACKETBUF_ATTR_RSSI, rx_rssi);

    /* Store link quality indicator */
    packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, rx_lqi);

    /* Reset rx_size to indicate no packet is pending */
    rx_size = 0;
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

  TRACE("on");

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

  TRACE("off");

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

  TRACE("get energy level");

  /* Turn on radio when necessary */
  if (!phyRxState) {
    phyTrxSetState(TRX_CMD_RX_ON);
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
  }

  /* Convert raw value to dBm and return it */
  return ed + RSSI_BASE_VAL;
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

  /* When asleep we don't handle interrupts */
  if (PHY_STATE_SLEEP == phyState) {
    return;
  }

  /* Only handle TRX_END interrupt */
  if (trx_reg_read(IRQ_STATUS_REG) & (1 << TRX_END)) {
    /* The interrupt is either caused by a TX or RX
     * When PHY state is IDLE it is RX
     */
    if (PHY_STATE_IDLE == phyState) {
      /* Read and convert ED level to obtain RSSI measurement */
      rx_rssi = (int8_t)trx_reg_read(PHY_ED_LEVEL_REG) + RSSI_BASE_VAL;

      /* Read first byte from frame buffer which holds the frame size */
      trx_frame_read(&size, 1);

      /* Read again from the frame buffer and this time read size + 2 bytes
       * Size is read again and LQI is included which is stored in the last byte */
      trx_frame_read(rx_buffer, size + 2);

      /* Calculate size of the frame without CRC bytes */
      rx_size = size - PHY_CRC_SIZE;

      /* Store LQI */
      rx_lqi = rx_buffer[size + 1];

      /* Poll the process */
      process_poll(&samr21_rf_process);
    } else if (PHY_STATE_TX_WAIT_END == phyState) {
      /* Interrupt was caused by a TX, read TX result */
      trac_status = (trx_reg_read(TRX_STATE_REG) >> TRAC_STATUS) & 7;

      /* If the radio was on, turn it on again */
      if (phyRxState) {
        phyTrxSetState(TRX_CMD_RX_AACK_ON);
      }

      /* Update state */
      phyState = PHY_STATE_IDLE;
    }
  }
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

  /* Set CLKM to 8 MHz */
  trx_reg_write(TRX_CTRL_0_REG, 0x04);

  /* Auto generate CRC, Enable monitor IRQ status, always show interrupt in status register */
  trx_reg_write(TRX_CTRL_1_REG, (1 << TX_AUTO_CRC_ON) | (3 << SPI_CMD_MODE) | (1 << IRQ_MASK_MODE));

  /* Enable frame buffer protection and keep OQPSK_SCRAM_EN bit on its reset value */
  trx_reg_write(TRX_CTRL_2_REG, (1 << RX_SAFE_MODE) | (1 << OQPSK_SCRAM_EN));

  /* Enable antenna switch and use the antenna diversity algorithm */
//  trx_reg_write(ANT_DIV_REG, (1 << ANT_EXT_SW_EN) | (1 << ANT_DIV_EN));
  trx_reg_write(ANT_DIV_REG, (1 << ANT_EXT_SW_EN) | EXTERNAL_ANTENNA);

  /* Set crystal capacitance value */
  trx_reg_write(XOSC_CTRL_REG, (0xF << XTAL_MODE) | (RF_CAP_TRIM << XTAL_TRIM));

  /* Install transceiver interrupt handler */
  trx_irq_init(samr21_interrupt_handler);

  /* Enable TRX_END interrupt */
  trx_reg_write(IRQ_MASK_REG, (1 << TRX_END));

  /* Set random SEED for CSMA-CA */
  rnd_seed = get_rnd_value();

  /* Use random value as CSMA seed */
  trx_reg_write(CSMA_SEED_0_REG, rnd_seed & 0xff);
  trx_reg_write(CSMA_SEED_1_REG, trx_reg_read(CSMA_SEED_1_REG) | ((rnd_seed >> 8) & 0x07));

  /* Start a process for handling incoming RF packets */
  process_start(&samr21_rf_process, NULL);

  /* Log */
  INFO("at86rf233 initialized");

  return 0;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(samr21_rf_process, ev, data)
{
  PROCESS_BEGIN();

  TRACE("samr21_rf_process started");

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
