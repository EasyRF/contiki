#include <asf.h>

/* at86rf233 includes */
#include "trx_access.h"
#include "phy.h"
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


#define PHY_STATUS_UNKNOWN            255
#define SAMR21_MAX_TX_POWER_REG_VAL   0x1f

#define ON_BOARD_ANTENNA    0x01
#define EXTERNAL_ANTENNA    0x02

/*---------------------------------------------------------------------------*/

static const int tx_power_dbm[SAMR21_MAX_TX_POWER_REG_VAL] = {4, 3.7, 3.4, 3, 2.5, 2, 1, 0, -1, -2, -3, -4, -6, -8, -12, -17};

#define OUTPUT_POWER_MIN  tx_power_dbm[SAMR21_MAX_TX_POWER_REG_VAL - 1]
#define OUTPUT_POWER_MAX  tx_power_dbm[0]

static uint8_t initialized;
static uint8_t rx_on;

static uint8_t tx_buffer[PACKETBUF_SIZE];
static volatile uint8_t tx_status;

static uint8_t rx_buffer[PACKETBUF_SIZE];
static volatile uint8_t rx_size;
static volatile uint8_t rx_lqi;
static volatile int8_t rx_rssi;

/*---------------------------------------------------------------------------*/
PROCESS(samr21_rf_process, "SAMR21 RF driver");
/*---------------------------------------------------------------------------*/
static void
gpio_init(void)
{
  struct port_config pin_conf;

  port_get_config_defaults(&pin_conf);
  pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
  port_pin_set_config(AT86RFX_SPI_SCK, &pin_conf);
  port_pin_set_config(AT86RFX_SPI_MOSI, &pin_conf);
  port_pin_set_config(AT86RFX_SPI_CS, &pin_conf);
  port_pin_set_config(AT86RFX_RST_PIN, &pin_conf);
  port_pin_set_config(AT86RFX_SLP_PIN, &pin_conf);
  port_pin_set_output_level(AT86RFX_SPI_SCK, true);
  port_pin_set_output_level(AT86RFX_SPI_MOSI, true);
  port_pin_set_output_level(AT86RFX_SPI_CS, true);
  port_pin_set_output_level(AT86RFX_RST_PIN, true);
  port_pin_set_output_level(AT86RFX_SLP_PIN, true);

  pin_conf.direction  = PORT_PIN_DIR_INPUT;
  port_pin_set_config(AT86RFX_SPI_MISO, &pin_conf);

  /* Enable APB Clock for RFCTRL */
  PM->APBCMASK.reg |= (1<<PM_APBCMASK_RFCTRL_Pos);

  /* Unclear ?? */
  REG_RFCTRL_FECFG = RFCTRL_CFG_ANT_DIV;
  struct system_pinmux_config config_pinmux;
  system_pinmux_get_config_defaults(&config_pinmux);
  config_pinmux.mux_position = MUX_PA09F_RFCTRL_FECTRL1;
  config_pinmux.direction    = SYSTEM_PINMUX_PIN_DIR_OUTPUT;
  system_pinmux_pin_set_config(PIN_RFCTRL1, &config_pinmux);
  system_pinmux_pin_set_config(PIN_RFCTRL2, &config_pinmux);
}
/*---------------------------------------------------------------------------*/
static int
init(void)
{
  uint16_t seed;

  INFO("Initializing at86rf233 transceiver");

  gpio_init();

  /* Initialize transceiver */
  PHY_Init();

  /* Enable antenna switch and select antenna 1 */
  trx_reg_write(ANT_DIV_REG, (1 << ANT_EXT_SW_EN) | ON_BOARD_ANTENNA);

  /* Enable RX SAFE mode */
  trx_reg_write(TRX_CTRL_2_REG, (1 << RX_SAFE_MODE));

  /* Set random SEED for CSMA-CA */
  seed = PHY_RandomReq();
  trx_reg_write(CSMA_SEED_0_REG, seed & 0xff);
  trx_reg_write(CSMA_SEED_1_REG, trx_reg_read(CSMA_SEED_1_REG) | ((seed >> 8) & 0x07));

  /* Install transceiver interrupt handler */
  trx_irq_init(PHY_TaskHandler);

  /* Enable TRX_END interrupt */
  trx_reg_write(IRQ_MASK_REG, (1 << TRX_END));

  /* Set default channel */
  PHY_SetChannel(SAMR21_RF_CHANNEL);

  /* Start a process for handling incoming RF packets */
  process_start(&samr21_rf_process, NULL);

  initialized = 1;

  /* Notify user */
  INFO("at86rf233 initialized");

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
receiving_packet(void)
{
  uint8_t status = trx_reg_read(TRX_STATUS_REG) & TRX_STATUS_MASK;
  int res = (status == TRX_STATUS_BUSY_RX || status == TRX_STATUS_BUSY_RX_AACK);
  if (res) WARN("receiving");
  return res;
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{
  int res = (rx_size > 0);
  if (res) WARN("pending");
  return res;
}
/*---------------------------------------------------------------------------*/
static int
prepare(const void *payload, unsigned short payload_len)
{
  /* Set payload length in first byte of tx_buffer */
  tx_buffer[0] = payload_len;

  /* Copy payload to tx_buffer */
  memcpy(&tx_buffer[1], payload, payload_len);

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
transmit(unsigned short transmit_len)
{
  /* Reset tx_status */
  tx_status = PHY_STATUS_UNKNOWN;

  /* Make sure we're not receiving a packet */
  if (receiving_packet() || pending_packet()) {
    WARN("Aborting transmit");
    return RADIO_TX_COLLISION;
  }

  /* PHY_DataReq expects the payload length in the first byte */
  PHY_DataReq(tx_buffer);

  TRACE("transmitting %d bytes", transmit_len);

  /* Wait for tx result */
  while (tx_status == PHY_STATUS_UNKNOWN);

  if (tx_status == PHY_STATUS_SUCCESS) {
    TRACE("RADIO_TX_OK");
    return RADIO_TX_OK;
  } else if (tx_status == PHY_STATUS_NO_ACK) {
    WARN("RADIO_TX_NOACK");
    return RADIO_TX_NOACK;
  } else if (tx_status == PHY_STATUS_CHANNEL_ACCESS_FAILURE) {
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
  prepare(payload, payload_len);
  return transmit(payload_len);
}
/*---------------------------------------------------------------------------*/
static int
read(void *buf, unsigned short buf_len)
{
  int len = rx_size;

  if (len > 0) {
    /* Check available buffer space */
    if (buf_len < len) {
      WARN("Packet does not fit buffer");
      return 0;
    }

    TRACE("received: %d bytes, rssi: %d", len, rx_rssi);

    /* Store the length of the packet */
    packetbuf_set_datalen(len);

    /* Store data of the packet buffer */
    memcpy(buf, rx_buffer, len);

    /* Store RSSI in dBm for the received packet */
    packetbuf_set_attr(PACKETBUF_ATTR_RSSI, rx_rssi);

    /* Store link quality indicator */
    packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, rx_lqi);

    /* Reset rx_size to indicate no packet is pending */
    rx_size = 0;
  }

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
  TRACE("on");

  while (receiving_packet());

  PHY_SetRxState(true);

  rx_on = 1;

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  TRACE("off");

  while (receiving_packet());

  PHY_SetRxState(false);

  rx_on = 0;

  return 0;
}
/*---------------------------------------------------------------------------*/
static uint8_t
get_channel(void)
{
  return (trx_reg_read(PHY_CC_CCA_REG) & SAMR21_CCA_CHANNEL_MASK);
}
/*---------------------------------------------------------------------------*/
static uint16_t
get_pan_id(void)
{
  uint16_t pan_id;
  uint8_t *d = (uint8_t *)&pan_id;

  d[0] = trx_reg_read(PAN_ID_0_REG);
  d[1] = trx_reg_read(PAN_ID_1_REG);

  return pan_id;
}
/*---------------------------------------------------------------------------*/
static uint16_t
get_short_addr(void)
{
  uint16_t short_id;
  uint8_t *d = (uint8_t *)&short_id;

  d[0] = trx_reg_read(SHORT_ADDR_0_REG);
  d[1] = trx_reg_read(SHORT_ADDR_1_REG);

  return short_id;
}
/*---------------------------------------------------------------------------*/
/* Returns the current TX power in dBm */
static radio_value_t
get_tx_power(void)
{
  uint8_t tmp = trx_reg_read(PHY_TX_PWR_REG) & 0x0f;
  if (tmp > SAMR21_MAX_TX_POWER_REG_VAL) {
    WARN("Invalid power mode, returning max value");
    return tx_power_dbm[0];
  } else {
    return tx_power_dbm[tmp];
  }
}
/*---------------------------------------------------------------------------*/
/* Returns the current CCA threshold in dBm */
/* P_THRES[dBm] = RSSI_BASE_VAL[dBm] + 2[dB] x CCA_ED_THRES */
static radio_value_t
get_cca_threshold(void)
{
  uint8_t cca_ed_thres = trx_reg_read(CCA_THRES_REG) & 0x0f;
  return PHY_RSSI_BASE_VAL + 2 * cca_ed_thres;
}
/*---------------------------------------------------------------------------*/
/* Set the current CCA threshold in dBm */
/* CCA_ED_THRES = (P_THRES[dBm] - RSSI_BASE_VAL[dBm]) / 2 */
static void
set_cca_threshold(radio_value_t value)
{
  uint8_t cca_ed_thres = (value - PHY_RSSI_BASE_VAL) / 2;
  trx_reg_write(CCA_THRES_REG, cca_ed_thres & 0x0f);
}
/*---------------------------------------------------------------------------*/
static void
set_tx_power(radio_value_t requested_tx_power)
{
  uint8_t i;

  /* Iterate the power modes in increasing order,
   * stop when power exceeds the requested value
   */
  for (i = SAMR21_MAX_TX_POWER_REG_VAL - 1; i >= 0; i--) {
    if (tx_power_dbm[i] > requested_tx_power) {
      break;
    }
  }

  PHY_SetTxPower(i);
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_value(radio_param_t param, radio_value_t *value)
{
  if(!value) {
    return RADIO_RESULT_INVALID_VALUE;
  }

  switch(param) {
  case RADIO_PARAM_POWER_MODE:
    *value = rx_on == 0 ? RADIO_POWER_MODE_OFF : RADIO_POWER_MODE_ON;
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
    while (receiving_packet());
    *value = PHY_EdReq();
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
    PHY_SetChannel(value);
    return RADIO_RESULT_OK;
  case RADIO_PARAM_PAN_ID:
    PHY_SetPanId(value & 0xffff);
    return RADIO_RESULT_OK;
  case RADIO_PARAM_16BIT_ADDR:
    PHY_SetShortAddr(value & 0xffff);
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

  if(param == RADIO_PARAM_64BIT_ADDR) {
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

  if(param == RADIO_PARAM_64BIT_ADDR) {
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
/*---------------------------------------------------------------------------*/
void
PHY_DataConf(uint8_t status)
{
  /* Transmit callback */

  /* Store transmit result, which will unblock the transmit function */
  tx_status = status;
}
/*---------------------------------------------------------------------------*/
void
PHY_DataInd(PHY_DataInd_t *ind)
{
  /* Receive callback */

  /* Copy rx data and attributes */
  if (ind->size > 0) {
    rx_size = ind->size;
    rx_rssi = ind->rssi;
    rx_lqi  = ind->lqi;
    memcpy(rx_buffer, ind->data, rx_size);
  }

  /* Poll the process */
  process_poll(&samr21_rf_process);
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
unsigned short
samr21_random_rand(void)
{
  if (initialized) {
    TRACE("samr21_random_rand");
    while (receiving_packet());
    return PHY_RandomReq();
  } else {
    WARN("PHY not initialized returning 0 instead of random number");
    return 0;
  }
}
