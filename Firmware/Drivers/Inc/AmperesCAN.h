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
 * NOTE: Amperes CAN message is held in bps_pack_current_t
 * - I'm using Main_Battery_Current_RawV internally to hold the ADC value;
 *   it gets converted to actual voltage value inside Amperes_SendCAN
 */

/**
 * Macros to reconstruct values
 * - little endian, see bps_pack_current_t struct. 
 * - handle sign extension for 24b -> int32_t
 */
#define AMPERES_UNPACK_CURRENT_mA(x)    ( (int32_t)(((uint32_t)(x)[3] << 24) | ((uint32_t)(x)[2] << 16) | ((uint32_t)(x)[1] << 8)) >> 8 )
#define AMPERES_UNPACK_RAW_mV(x)        ( (uint16_t)((x[2] << 8) | (uint16_t) x[1]) )

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
 * @brief Send Amperes current data over BPS_CAN
 * @param data Pointer to bps_pack_current_t message struct
 * @param ticksToWait Number of ticks to wait on send: 0 for non-blocking, portMAX_DELAY for blocking
 * @retval AmperesStatus_t
 * @n 
 * - AMPERES_OK on success
 * @n
 * - AMPERES_CAN_SEND_FAIL on fail
 */
AmperesStatus_t Amperes_SendPackCurrentCAN(bps_pack_current_t *data, TickType_t ticksToWait);

/**
 * @brief Send Amperes rawMv data over BPS_CAN
 * @param data Pointer to bps_pack_current_rawv_t message struct
 * @param ticksToWait Number of ticks to wait on send: 0 for non-blocking, portMAX_DELAY for blocking
 * @retval AmperesStatus_t
 * @n 
 * - AMPERES_OK on success
 * @n
 * - AMPERES_CAN_SEND_FAIL on fail
 */
AmperesStatus_t Amperes_SendPackCurrentRaw(bps_pack_current_rawv_t *data, TickType_t ticksToWait);
