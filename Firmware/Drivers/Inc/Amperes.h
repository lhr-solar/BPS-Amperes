#pragma once

#include "common.h"
#include "ADC.h"
#include "CAN.h"
#include "BPSCAN_can_msgs.h"
#include "AmperesConfig.h"


/** ================================================================
 *  ADC (12 bit)
 * ================================================================ */
extern QueueHandle_t adc_queue;


/** ================================================================
 *  Calibration + Conversion
 * ================================================================ */

/**
 * Calibrated values (using shunt).
 * - ADC_xA is the ADC value at different current values.
 * - Scale constant is just the slope: (current1 - current2) / (adc1 - adc2)
 */
#define ADC_0A          1527
#define ADC_PLUS_10A    1828
#define ADC_NEG_10A     1228
#define SCALE_CONST     33333

// Noise range in mA (arbitrary right now)
#define NOISE_RANGE     35

/**
 * @brief Convert ADC reading to current in milliamps (mA) using calibrated values.
 * @param adc_val 12 bit ADC value to be converted
 * @retval Amperes current reading in mA
 */
int32_t Amperes_ADCToCurrent(uint16_t adc_val);


/** ================================================================
 *  CAN
 * ================================================================ */


/** ================================================================
 *  UART (USART1)
 * ================================================================ */


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
