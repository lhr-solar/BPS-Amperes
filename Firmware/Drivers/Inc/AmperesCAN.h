#pragma once

#include "CAN.h"
#include "AmperesConfig.h"
#include "BPSCAN_can_msgs.h"


/** ================================================================
 *  CAN
 * ================================================================ */
#define CAN_TX_ITEM_SIZE sizeof(can_tx_payload_t)
#ifndef CAN_TX_QUEUE_LENGTH
    #define CAN_TX_QUEUE_LENGTH 20
#endif
extern QueueHandle_t can_tx_queue;

/**
 * @brief Structure to hold Amperes data
 * @n 
 * - int32_t current_data
 * @n 
 * - uint16_t adc_voltage
 */
typedef struct AmperesMsg {
    int32_t current_data;   // signed, 32 bit
    uint16_t adc_voltage;   // unsigned, 12 bit
} AmperesMsg_t;

/**
 * @brief Initialize CAN filter and hardware
 * @retval AmperesStatus_t
 * @n 
 * - AMPERES_OK on success 
 * @n 
 * - AMPERES_INIT_FAIL on fail
 */
AmperesStatus_t Amperes_CAN_Init();

/**
 * @brief Start CAN peripheral
 * @retval AmperesStatus_t
 * @n 
 * - AMPERES_OK on success 
 * @n 
 * - AMPERES_INIT_FAIL on fail
 */
AmperesStatus_t Amperes_CAN_Start();

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
