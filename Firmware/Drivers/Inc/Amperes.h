#pragma once

#include "common.h"
#include "ADC.h"
#include "CAN.h"
#include "BPSCAN_can_msgs.h"

/** ================================================================
 *  Driver for Amperes Board
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
extern QueueHandle_t adc_queue;


/** ================================================================
 *  ADC Conversion: Fixed point math
 * ================================================================ */

/**
 * ================================
 * Conversion
 * ================================
 * Equation:
 * mA = [(ADC_val - ADC_Vref) * adc_voltage_range_mV] / [adc_range * shunt*gain]
 * 
 * Plug in values:
 * mA = [(ADC_val - (1250)(4095/3300)) * 3300] / [4095 * 0.0249]
 * 
 * Simplify:
 * mA = [(ADC_val - 1551.13636364) * 3300] / [101.9655]
 * 
 * Scale values:
 * mA = [(ADC_val*1000 - 1551.13636364*1000) * 3300*10] / [101.9655*10000]
 *
 * Final equation:
 * mA = [(ADC_val*1000 - 1551136) * 33000] / [1019655]
 * 
 * ================================
 * Constants
 * ================================
 * ADC Vref Scaled = 1551136
 * Numerator Scaled = 33000
 * Denominator Scaled = 1019655
 */

/**
 * Reference voltage (1250 mV) in terms of ADC value, scaled by 1000.
 * - (1250 mV * 4095 * 1000) / 3300 mV ~= 1551136
 */
#define AMPERES_ADC_VREF_SCALED 1551136

/**
 * Numerator Term: 3300 * 10 = 33000
 */
#define AMPERES_CONV_NUM_SCALED 33000

/**
 * Denominator Term: 4095 * 0.0249 * 10000 = 1019655
 */
#define AMPERES_CONV_DEN_SCALED 1019655

/**
 * @brief Convert ADC reading to current in milliamps (mA).
 * @n 
 * mA = [(ADC_val - ADC_Vref) * 3300] / [4095 * 0.0249]
 * @n
 * - Internally scales values to the order of 10^6 to preserve decimal places during integer division.
 * @n
 * - 0.0249 is (shunt * gain).
 * @param adc_val 12 bit ADC value to be converted
 * @retval Amperes current reading in mA
 */
int32_t Amperes_ADCToCurrent(uint16_t adc_val);


/** ================================================================
 *  CAN
 * ================================================================ */
#define AMPERES_MSG_DLC     6
#define AMPERES_CAN_PORT    GPIOB
#define AMPERES_RX_PIN      GPIO_PIN_8
#define AMPERES_TX_PIN      GPIO_PIN_9

/**
 * @brief Structure to hold Amperes data
 * @n 
 * - int32_t current_data
 * @n 
 * - uint16_t adc_voltage
 */
typedef struct {
    int32_t current_data;   // signed, 32 bit
    uint16_t adc_voltage;   // unsigned, 12 bit
} AmperesMsg_t;


/** ================================================================
 *  Amperes Functions
 * ================================================================ */
typedef enum AmperesStatus {
    AMPERES_OK,
    AMPERES_INIT_FAIL,
    AMPERES_ADC_START_FAIL,
    AMPERES_ADC_READ_FAIL,
    AMPERES_CAN_SEND_FAIL
} AmperesStatus_t;

/**
 * @brief Initializes ADC and CAN for Amperes
 * @retval AmperesStatus_t
 * @n 
 * - AMPERES_OK on success 
 * @n 
 * - AMPERES_INIT_FAIL on fail
 */
AmperesStatus_t Amperes_Init();

/**
 * @brief Start ADC reading
 * @param clearQueue Reset queue (if blocking on [queue empty -> has data] condition)
 * @retval AmperesStatus_t
 * @n 
 * - AMPERES_OK on success
 * @n 
 * - AMPERES_ADC_START_FAIL on fail
 */
AmperesStatus_t Amperes_StartADC(bool clearQueue);

/**
 * @brief Get adc reading from ADC queue and convert to current (mA);
 *        expected range is -50.000 to +82.00 amps.
 * @param message Pointer to AmperesMsg_t struct to hold adc and current values
 * @param ticksToWait Number of ticks to wait on queue: 0 for non-blocking, portMAX_DELAY for blocking
 * @retval AmperesStatus_t
 * @n 
 * - AMPERES_OK on success
 * @n
 * - AMPERES_ADC_READ_FAIL on fail
 */
AmperesStatus_t Amperes_GetReading(AmperesMsg_t *message, TickType_t ticksToWait);

/**
 * @brief Send Amperes data over BPS_CAN
 * @param data Pointer to Amperes message struct
 * @param ticksToWait Number of ticks to wait on send: 0 for non-blocking, portMAX_DELAY for blocking
 * @retval AmperesStatus_t
 * @n 
 * - AMPERES_OK on success
 * @n
 * - AMPERES_CAN_SEND_FAIL on fail
 */
AmperesStatus_t Amperes_SendCAN(AmperesMsg_t *data, TickType_t ticksToWait);

/**
 * @brief Updates Amperes LED (Charge, Discharge) based on current measurement.
 * @param currentValue Current measurement value
 * @retval None
 */
void Amperes_UpdateLEDs(int32_t currentValue);