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
 * NOTE: the Amperes pack-current message is held in bps_pack_current_t.
 * The raw measurement is now sent as 12-bit ADC counts (0-4095) in
 * Main_Battery_Current_ADC on CAN_ID_BPS_PACK_CURRENT_ADC (no mV conversion).
 */

/**
 * @brief Unpack rx_data array into bps_pack_current_t message
 * (BPS_Amperes_Fault, Main_Battery_Current, FrameID_Amperes fields)
 * @param rx_data Receive message array
 * @param result Pointer to bps_pack_current_t struct to store result; MUST BE CORRECT SIZE
 */
void Amperes_Unpack_Current_mA(uint8_t rx_data[], bps_pack_current_t *result);

/**
 * @brief Unpack rx_data array into bps_pack_current_adc_t message
 * (Main_Battery_Current_ADC, FrameID_Amperes fields)
 * @param rx_data Receive message array
 * @param result Pointer to bps_pack_current_adc_t struct to store result; MUST BE CORRECT SIZE
 */
void Amperes_Unpack_ADC(uint8_t rx_data[], bps_pack_current_adc_t *result);


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
 * @brief Send Amperes raw ADC-count data over BPS_CAN
 * @param data Pointer to bps_pack_current_adc_t message struct
 * @param ticksToWait Number of ticks to wait on send: 0 for non-blocking, portMAX_DELAY for blocking
 * @retval AmperesStatus_t
 * @n 
 * - AMPERES_OK on success
 * @n
 * - AMPERES_CAN_SEND_FAIL on fail
 */
AmperesStatus_t Amperes_SendPackCurrentADC(bps_pack_current_adc_t *data, TickType_t ticksToWait);
