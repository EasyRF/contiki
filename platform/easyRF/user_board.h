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

#ifndef USER_BOARD_H
#define USER_BOARD_H

#include <conf_board.h>
#include <compiler.h>

#ifdef __cplusplus
extern "C" {
#endif


void system_board_init(void);


/** Name string macro */
#define BOARD_NAME                "EasyRF-Gateway"


/** \name USB definitions
 * @{
 */
#define USB_ID
#define USB_TARGET_DP_PIN            PIN_PA25G_USB_DP
#define USB_TARGET_DP_MUX            MUX_PA25G_USB_DP
#define USB_TARGET_DP_PINMUX         PINMUX_PA25G_USB_DP
#define USB_TARGET_DM_PIN            PIN_PA24G_USB_DM
#define USB_TARGET_DM_MUX            MUX_PA24G_USB_DM
#define USB_TARGET_DM_PINMUX         PINMUX_PA24G_USB_DM
#define USB_VBUS_PIN                 PIN_PA07
#define USB_VBUS_EIC_LINE            7
#define USB_VBUS_EIC_MUX             MUX_PA07A_EIC_EXTINT7
#define USB_VBUS_EIC_PINMUX          PINMUX_PA07A_EIC_EXTINT7
/* USB ID pin is not connected */
//#define USB_ID_PIN                   -1
//#define USB_ID_EIC_LINE              -1
//#define USB_ID_EIC_MUX               -1
//#define USB_ID_EIC_PINMUX            -1
/** @} */


/** \name LEDS Pin definitions
 * @{
 */

#define LED_RED_PIN     PIN_PA15
#define LED_GREEN_PIN   PIN_PA16
#define LED_BLUE_PIN    PIN_PA17
#define LED_WHITE_PIN   PIN_PA27

/** @} */

/** \name Ethernet/SerialFlash/Display SPI Interface definitions
 * @{
 */

#define ESD_SPI_MODULE                SERCOM3
#define ESD_SPI_SERCOM_MUX_SETTING    SPI_SIGNAL_MUX_SETTING_E // TODO: Check this
#define ESD_SPI_SERCOM_PINMUX_PAD0    PINMUX_PA22C_SERCOM3_PAD0
#define ESD_SPI_SERCOM_PINMUX_PAD1    PINMUX_UNUSED
#define ESD_SPI_SERCOM_PINMUX_PAD2    PINMUX_PA18D_SERCOM3_PAD2
#define ESD_SPI_SERCOM_PINMUX_PAD3    PINMUX_PA19D_SERCOM3_PAD3

#define ETHERNET_CS                   PIN_PA23
#define ETHERNET_IRQ_PIN              PIN_PA23
#define ETHERNET_IRQ_PINMUX           PINMUX_PA23A_EIC_EXTINT7
#define ETHERNET_IRQ_CHANNEL          7

#define SERIAL_FLASH_CS               PIN_PA27
#define DISPLAY_CS                    PIN_PA28
#define DISPLAY_CMD_DATA_PIN          PIN_PA22
#define DISPLAY_BACKLIGHT_PIN         PIN_PA14

#define ESD_SPI_BAUDRATE              2000000UL

/** @} */


/** \name RS-232 port definitions
 * @{
 */

#define RS232_MODULE                SERCOM5
#define RS232_SERCOM_MUX_SETTING    USART_RX_3_TX_2_XCK_3
#define RS232_SERCOM_PINMUX_PAD0    PINMUX_UNUSED
#define RS232_SERCOM_PINMUX_PAD1    PINMUX_UNUSED
#define RS232_SERCOM_PINMUX_PAD2    PINMUX_PB22D_SERCOM5_PAD2
#define RS232_SERCOM_PINMUX_PAD3    PINMUX_PB23D_SERCOM5_PAD3

/** @} */


/** \name RS-485 port definitions
 * @{
 */

#define RS485_MODULE                SERCOM1
#define RS485_SERCOM_MUX_SETTING    USART_RX_1_TX_0_XCK_1
#define RS485_SERCOM_PINMUX_PAD0    PINMUX_PA00D_SERCOM1_PAD0
#define RS485_SERCOM_PINMUX_PAD1    PINMUX_PA01D_SERCOM1_PAD1
#define RS485_SERCOM_PINMUX_PAD2    PINMUX_UNUSED
#define RS485_SERCOM_PINMUX_PAD3    PINMUX_UNUSED

#define RS485_TXE                   PIN_PB03

/** @} */


/** \name Light/Color/Proximity Sensor, Pressure Sensor, Inertial/Movement/Magneto I2C Interface definitions
 * @{
 */

#define SENSORS_I2C_MODULE                SERCOM2
#define SENSORS_I2C_SERCOM_PINMUX_PAD0    PINMUX_PA08D_SERCOM2_PAD0
#define SENSORS_I2C_SERCOM_PINMUX_PAD1    PINMUX_PA13C_SERCOM2_PAD1

/** @} */


/** \name Infrared transceiver Interface definitions
 * @{
 */

#define INFRARED_TX_MODULE                SERCOM0
#define INFRARED_TX_SERCOM_MUX_SETTING    USART_RX_0_TX_0_XCK_1
#define INFRARED_TX_SERCOM_PINMUX_PAD0    PINMUX_PA04_SERCOM0_PAD0
#define INFRARED_TX_SERCOM_PINMUX_PAD1    PINMUX_UNUSED
#define INFRARED_TX_SERCOM_PINMUX_PAD2    PINMUX_UNUSED
#define INFRARED_TX_SERCOM_PINMUX_PAD3    PINMUX_UNUSED

#define INFRARED_RX_MODULE                SERCOM1 /* Shared with RS-485. Mutually exclusive. */
#define INFRARED_RX_SERCOM_MUX_SETTING    USART_RX_2_TX_2_XCK_3
#define INFRARED_RX_SERCOM_PINMUX_PAD0    PINMUX_UNUSED
#define INFRARED_RX_SERCOM_PINMUX_PAD1    PINMUX_UNUSED
#define INFRARED_RX_SERCOM_PINMUX_PAD2    PINMUX_PA30_SERCOM1_PAD2
#define INFRARED_RX_SERCOM_PINMUX_PAD3    PINMUX_UNUSED

/** @} */


/** \name QTouch timer definitions
 * @{
 */

#define TOUCH_TC_MODULE   TC4

/** @} */


/** \name 802.15.4 TRX Interface definitions
 * @{
 */

#define RF_SPI_MODULE              SERCOM4
#define RF_SPI_SERCOM_MUX_SETTING  SPI_SIGNAL_MUX_SETTING_E
#define RF_SPI_SERCOM_PINMUX_PAD0  PINMUX_PC19F_SERCOM4_PAD0
#define RF_SPI_SERCOM_PINMUX_PAD1  PINMUX_PB31D_SERCOM5_PAD1
#define RF_SPI_SERCOM_PINMUX_PAD2  PINMUX_PB30F_SERCOM4_PAD2
#define RF_SPI_SERCOM_PINMUX_PAD3  PINMUX_PC18F_SERCOM4_PAD3


#define RF_IRQ_MODULE           EIC
#define RF_IRQ_INPUT            0
#define RF_IRQ_PIN              PIN_PB00A_EIC_EXTINT0
#define RF_IRQ_MUX              MUX_PB00A_EIC_EXTINT0
#define RF_IRQ_PINMUX           PINMUX_PB00A_EIC_EXTINT0

#define AT86RFX_SPI                  SERCOM4
#define AT86RFX_RST_PIN              PIN_PB15
#define AT86RFX_IRQ_PIN              PIN_PB00
#define AT86RFX_SLP_PIN              PIN_PA20
#define AT86RFX_SPI_CS               PIN_PB31
#define AT86RFX_SPI_MOSI             PIN_PB30
#define AT86RFX_SPI_MISO             PIN_PC19
#define AT86RFX_SPI_SCK              PIN_PC18
#define PIN_RFCTRL1                  PIN_PA09
#define PIN_RFCTRL2                  PIN_PA12
#define RFCTRL_CFG_ANT_DIV           4

#define AT86RFX_SPI_SERCOM_MUX_SETTING    RF_SPI_SERCOM_MUX_SETTING
#define AT86RFX_SPI_SERCOM_PINMUX_PAD0    RF_SPI_SERCOM_PINMUX_PAD0
#define AT86RFX_SPI_SERCOM_PINMUX_PAD1    PINMUX_UNUSED
#define AT86RFX_SPI_SERCOM_PINMUX_PAD2    RF_SPI_SERCOM_PINMUX_PAD2
#define AT86RFX_SPI_SERCOM_PINMUX_PAD3    RF_SPI_SERCOM_PINMUX_PAD3

#define AT86RFX_IRQ_CHAN                  RF_IRQ_INPUT
#define AT86RFX_IRQ_PINMUX                RF_IRQ_PINMUX


/** Enables the transceiver main interrupt. */
#define ENABLE_TRX_IRQ()    \
    extint_chan_enable_callback(AT86RFX_IRQ_CHAN, EXTINT_CALLBACK_TYPE_DETECT)

/** Disables the transceiver main interrupt. */
#define DISABLE_TRX_IRQ()   \
    extint_chan_disable_callback(AT86RFX_IRQ_CHAN, EXTINT_CALLBACK_TYPE_DETECT)

/** Clears the transceiver main interrupt. */
#define CLEAR_TRX_IRQ()     \
    extint_chan_clear_detected(AT86RFX_IRQ_CHAN);

/*
 * This macro saves the trx interrupt status and disables the trx interrupt.
 */
#define ENTER_TRX_REGION()   \
    { extint_chan_disable_callback(AT86RFX_IRQ_CHAN, EXTINT_CALLBACK_TYPE_DETECT)

/*
 *  This macro restores the transceiver interrupt status
 */
#define LEAVE_TRX_REGION()   \
    extint_chan_enable_callback(AT86RFX_IRQ_CHAN, EXTINT_CALLBACK_TYPE_DETECT); }

/** @} */


#ifdef __cplusplus
}
#endif

#endif // USER_BOARD_H
