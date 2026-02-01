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
#define AMPERES_SAMPLE_TIME ADC_SAMPLETIME_2CYCLES_5
// ADC_SAMPLETIME_47CYCLES_5
#define AMPERES_ADC_PORT    GPIOA
#define AMPERES_ADC_PIN     GPIO_PIN_0

// Reference voltage for bidirectional current reading (1.25V)
#define AMPERES_mVREF 1250

extern QueueHandle_t adc_queue;


/** ================================================================
 *  CAN
 * ================================================================ */
#define AMPERES_STD_ID 0x1
extern QueueHandle_t can_queue;

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

typedef enum AmperesStatus {    // TODO: better states
    AMPERES_OK,
    AMPERES_INIT_FAIL,
    AMPERES_ADC_START_FAIL,
    AMPERES_ADC_READ_FAIL,
    AMPERES_QUEUE_FULL,
    AMPERES_CAN_SEND_FAIL
} AmperesStatus_t;


/**
 * @brief Initializes ADC and CAN for Amperes
 * @param timerDriven True if you want to trigger ADC conversion from hardware Timer6.
 *                    False if you want to manually poll ADC.
 * @retval Status: AMPERES_INIT_FAIL or OK
 */
AmperesStatus_t Amperes_Init(bool timerDriven);


/**
 * @brief Convert ADC reading to current (in milliamps).
 * @n 
 *  - ADC to mV: ADC * (3300 mV / 4096)
 * @n 
 *  - mV to mA: (mV - mVref) / ((250 E-6 ohm)(100 V/V))
 * @n 
 * - ADC to mA: (reading * (3300 / 4096)) - mVref) / (0.025)
 * @param reading 12 bit ADC value to be converted
 * @retval Amperes current reading in mA
 */
int32_t Amperes_ADCToCurrent(uint16_t reading);


/**
 * @brief Start ADC reading
 * @retval Amperes Status: AMPERES_ADC_START_FAIL or OK
 */
AmperesStatus_t Amperes_StartADC();

/**
 * @brief Get adc reading from ADC queue and convert to current (mA);
 *        expected range is -50.000 to +82.00 amps.
 * @param current_reading Variable to hold current reading (int32_t)
 * @param adc_reading Variable to holds raw ADC reading (uint16_t)
 * @retval Amperes Status: AMPERES_ADC_READ_FAIL or OK
 */
AmperesStatus_t Amperes_GetReading(AmperesMsg_t *message);


/**
 * @brief Send Amperes data over BPS_CAN
 * @param data Pointer to Amperes message struct
 * @retval Amperes Status: AMPERES_CAN_SEND_FAIL or OK
 */
AmperesStatus_t Amperes_SendCAN(AmperesMsg_t *data);

#endif // AMPERES_H