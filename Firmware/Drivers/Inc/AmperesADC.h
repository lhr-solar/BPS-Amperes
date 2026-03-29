#pragma once

#include "ADC.h"
#include "AmperesConfig.h"
#include "AmperesCAN.h"

/** ================================================================
 *  ADC
 * ================================================================ */
#define ADC_ITEM_SIZE sizeof(uint16_t)
#ifndef ADC_QUEUE_LENGTH
    #define ADC_QUEUE_LENGTH 2
#endif
extern QueueHandle_t adc_queue;

/** ==============================================================
 * Calibration and conversion
 * ===============================================================
 * - ADC_xA is the ADC value at different current values.
 * - Scale constant is just the slope scaled by 1000: 
 *      (mA1 - mA2) / (adc1 - adc2) * 1000
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

/**
 * @brief Initialize ADC queue and hardware
 * @retval AmperesStatus_t
 * @n 
 * - AMPERES_OK on success 
 * @n 
 * - AMPERES_INIT_FAIL on fail
 */
AmperesStatus_t Amperes_ADC_Init();

/**
 * @brief Start ADC conversion
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
 *        expected range is -50,000 to +82,00 mA.
 * @param message Pointer to bps_pack_current_t struct to hold adc and current values
 * @param ticksToWait Number of ticks to wait on queue: 0 for non-blocking, portMAX_DELAY for blocking
 * @retval AmperesStatus_t
 * @n 
 * - AMPERES_OK on success
 * @n
 * - AMPERES_ADC_READ_FAIL on fail
 */
AmperesStatus_t Amperes_GetReading(bps_pack_current_t *message, TickType_t ticksToWait);
