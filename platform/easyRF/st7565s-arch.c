#include <asf.h>
#include "contiki.h"
#include "esd_spi_master.h"
#include "log.h"


static struct spi_slave_inst slave;


/*---------------------------------------------------------------------------*/
static void
configure_miso(bool as_output)
{
  if (as_output) {
    /* Configure MISO as output */
    struct port_config gp_pin_conf;
    port_get_config_defaults(&gp_pin_conf);
    gp_pin_conf.direction = PORT_PIN_DIR_OUTPUT;
    port_pin_set_config(DISPLAY_CMD_DATA_PIN, &gp_pin_conf);
  } else {
    /* Configure MISO as SPI pin */
    struct system_pinmux_config mux_pin_conf;
    system_pinmux_get_config_defaults(&mux_pin_conf);
    mux_pin_conf.mux_position = ESD_SPI_SERCOM_PINMUX_PAD0 & 0xFFFF;
    mux_pin_conf.direction = SYSTEM_PINMUX_PIN_DIR_INPUT;
    mux_pin_conf.input_pull = SYSTEM_PINMUX_PIN_PULL_UP;
    system_pinmux_pin_set_config(ESD_SPI_SERCOM_PINMUX_PAD0 >> 16, &mux_pin_conf);
  }
}
/*---------------------------------------------------------------------------*/
void
st7565s_arch_spi_init(void)
{
  struct spi_slave_inst_config slave_dev_config;

  /* Init SPI master interface */
  esd_spi_master_init();

  /* Congfigure slave SPI device */
  spi_slave_inst_get_config_defaults(&slave_dev_config);
  slave_dev_config.ss_pin = DISPLAY_CS;
  spi_attach_slave(&slave, &slave_dev_config);

  /* Configure backlight pin */
  struct port_config pin_conf;
  port_get_config_defaults(&pin_conf);
  /* Configure as output and turn off */
  pin_conf.direction = PORT_PIN_DIR_OUTPUT;
  port_pin_set_config(DISPLAY_BACKLIGHT_PIN, &pin_conf);
  port_pin_set_output_level(DISPLAY_BACKLIGHT_PIN, false);
}
/*---------------------------------------------------------------------------*/
uint8_t
st7565s_arch_spi_write(uint8_t data)
{
  uint16_t in;
  spi_transceive_wait(&spi_master_instance, data, &in);
  return in & 0xff;
}
/*---------------------------------------------------------------------------*/
void
st7565s_arch_spi_select(bool is_data)
{
  spi_select_slave(&spi_master_instance, &slave, true);

  configure_miso(true);

  port_pin_set_output_level(DISPLAY_CMD_DATA_PIN, is_data);
}
/*---------------------------------------------------------------------------*/
void
st7565s_arch_spi_deselect(void)
{
  port_pin_set_output_level(DISPLAY_CMD_DATA_PIN, false);

  configure_miso(false);

  spi_select_slave(&spi_master_instance, &slave, false);
}
/*---------------------------------------------------------------------------*/
void
st7565s_arch_set_backlight(bool on)
{
  port_pin_set_output_level(DISPLAY_BACKLIGHT_PIN, on);
}
