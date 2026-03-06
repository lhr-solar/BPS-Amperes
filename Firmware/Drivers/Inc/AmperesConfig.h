#pragma once

/** ================================================================
 *  Amperes Board
 *  ================================================================
 *  MCU: STM32L431CBT6
 *  ADC: PA0
 *  Operation: Receive data from ADC, convert to 
 *             current reading, and send over CAN.
 * 
 *  Specifications:
 *  - Current range: -46 to +78 A (0.1 - 3.2 V respectively)
 *  - Shunt: 250 uOhm
 *  - Gain: 100 * 0.4 * 2.49 V/V = 99.6 V/V
 *  - Reference voltage: 1.25 V (for bidirectional sense amp)
 *  - ADC: 12 bit
*/


 /** ================================================================
 *  GPIO
 * ================================================================ */

/* LED Pins */
#define AMPERES_GPIO_PORT       GPIOA
#define AMPERES_HB_PIN          GPIO_PIN_3
#define AMPERES_FAULT_PIN       GPIO_PIN_4
#define AMPERES_CHARGE_PIN      GPIO_PIN_5
#define AMPERES_DISCHARGE_PIN   GPIO_PIN_6

/* GPIO Pins */
#define AMPERES_BOOT_PIN        GPIO_PIN_7

/** ================================================================
 *  ADC (12 bit)
 * ================================================================ */

#define AMPERES_ADC_CHANNEL ADC_CHANNEL_5
#define AMPERES_SAMPLE_TIME ADC_SAMPLETIME_47CYCLES_5
#define AMPERES_ADC_PORT    GPIOA
#define AMPERES_ADC_PIN     GPIO_PIN_0

/** ================================================================
 *  CAN (CAN1)
 * ================================================================ */

#define AMPERES_MSG_DLC     6
#define AMPERES_CAN_PORT    GPIOB
#define AMPERES_RX_PIN      GPIO_PIN_8
#define AMPERES_TX_PIN      GPIO_PIN_9

/** ================================================================
 *  UART (USART1)
 * ================================================================ */

#define AMPERES_UART_PORT   GPIOA
#define AMPERES_UART_TX_PIN GPIO_PIN_9
#define AMPERES_UART_RX_PIN GPIO_PIN_10
