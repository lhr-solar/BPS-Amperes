#ifndef AMPERES_H
#define AMPERES_H

#include "common.h"
#include "ADC.h"
#include "CAN.h"

/** ==========================================================
 *  Driver for Amperes Board
 *  ==========================================================
 *  MCU: STM32L431CBT6
 *  ADC: PA0
 *  Operation: Receive data from ADC, convert to 
 *             current reading, and send over CAN.
 * ========================================================== */

/* LED Pins */
#define AMPERES_LED_PORT        GPIOA
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
#define AMPERES_SAMPLE_TIME ADC_SAMPLETIME_2CYCLES_5
// ADC_SAMPLETIME_47CYCLES_5
#define AMPERES_ADC_PORT    GPIOA
#define AMPERES_ADC_PIN     GPIO_PIN_0

// Reference voltage for bidirectional current reading (1.25V)
#define AMPERES_VREF 1.25
#define AMPERES_mVREF 1250

/**
 * @brief Convert ADC reading to current (in milliamps).
 * @n 
 *  - ADC to mV: ADC * (3300 mV / 4096)
 * @n 
 *  - mV to mA: (mV - mVref) / ((250 E-6 ohm)(100 V/V))
 * @param reading ADC value to be converted
 */
#define AMPERES_ADC_TO_CURRENT(reading) (((reading * (3300 / 4096)) - AMPERES_mVREF) / (0.025))



/** ================================================================
 *  CAN
 * ================================================================ */
#define AMPERES_STD_ID 0x1



/** ================================================================
 *  Amperes Functions
 * ================================================================ */

typedef enum AmperesStatus {    // TODO: better states
    AMPERES_OK,
    AMPERES_INIT_FAIL,
    AMPERES_ADC_FAIL,
    AMPERES_QUEUE_FULL,
    AMPERES_CAN_SEND_FAIL
} AmperesStatus_t;

/**
 * @brief Initializes ADC and CAN for Amperes
 * @retval Status: AMPERES_INIT_FAIL or OK
 */
AmperesStatus_t Amperes_Init();

/**
 * @brief Get current reading (in milliamps) from ADC;
 *        expected range is -50.000 to +82.00 amps.
 * @param current_reading Variable to hold current reading 
 * @retval Status: AMPERES_ADC_FAIL, QUEUE_FULL, or OK
 */
AmperesStatus_t Amperes_GetReading(int32_t *current_reading);


/**
 * @brief Send Amperes data over BPS_CAN
 * @param data Amperes current data to send over CAN
 * @retval Status: AMPERES_CAN_SEND_FAIL or OK
 */
AmperesStatus_t Amperes_SendCAN(int32_t data);

#endif // AMPERES_H