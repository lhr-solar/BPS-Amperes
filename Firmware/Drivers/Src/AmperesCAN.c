#include "AmperesCAN.h"

 /** ================================================================
 *  Local Init Function
 * ================================================================ */

/**
 * @brief Initializes CAN filter and hardware.
 * - Fails if can_init() fails.
 * - Called in Amperes_Init().
 */
static bool MX_CAN_Init() {
    /* Create CAN filter */
    /* For production, reject all incoming IDs */
    CAN_FilterTypeDef  sFilterConfig;
    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterActivation = DISABLE;

    /* For testing: accept all incoming IDs */
    // CAN_FilterTypeDef  sFilterConfig;
    // sFilterConfig.FilterBank = 0;
    // sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    // sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    // sFilterConfig.FilterIdHigh = 0x0000;
    // sFilterConfig.FilterIdLow = 0x0000;
    // sFilterConfig.FilterMaskIdHigh = 0x0000;
    // sFilterConfig.FilterMaskIdLow = 0x0000;
    // sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    // sFilterConfig.FilterActivation = ENABLE;
    // sFilterConfig.SlaveStartFilterBank = 14;

    /* CAN1 Init Struct */
    // Baud rate is 250 kbit/s
    hcan1->Init.Prescaler = 20;
    hcan1->Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan1->Init.TimeSeg1 = CAN_BS1_13TQ;
    hcan1->Init.TimeSeg2 = CAN_BS2_2TQ;
    hcan1->Init.Mode = CAN_MODE_LOOPBACK;   // TODO: change for testing
    hcan1->Init.TimeTriggeredMode = DISABLE;
    hcan1->Init.AutoBusOff = DISABLE;
    hcan1->Init.AutoWakeUp = DISABLE;
    hcan1->Init.AutoRetransmission = ENABLE;
    hcan1->Init.ReceiveFifoLocked = DISABLE;

    // If TransmitFifoPriority is disabled, the hardware selects the mailbox based on the message ID priority. 
    // If enabled, the hardware uses a FIFO mechanism to select the mailbox based on the order of transmission requests.
    hcan1->Init.TransmitFifoPriority = ENABLE;

    /* Initialize CAN1 */
    if (can_init(hcan1, &sFilterConfig) != CAN_OK) return false;
    
    return true;
}

/** ================================================================
 *  Amperes CAN Functions
 * ================================================================ */

AmperesStatus_t Amperes_CAN_Init() {
    // Init CAN
    if (!MX_CAN_Init()) return AMPERES_CAN_INIT_FAIL;
    return AMPERES_OK;
}

AmperesStatus_t Amperes_CAN_Start() {
    // Start CAN
    if (can_start(hcan1) != CAN_OK) return AMPERES_CAN_START_FAIL;
    return AMPERES_OK;
}

AmperesStatus_t Amperes_SendCAN(AmperesMsg_t *data, TickType_t ticksToWait) {
    // Create CAN payload
    CAN_TxHeaderTypeDef tx_header = {0};
    tx_header.StdId = AMPERES_MSG_ID;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.IDE = CAN_ID_STD;
    tx_header.DLC = AMPERES_MSG_DLC;
    tx_header.TransmitGlobalTime = DISABLE;

    // Split data into uint8 elements
    // Little Endian: LSB at index 0

    /* Raw ADC Value: uint16_t */
    uint8_t tx_data[AMPERES_MSG_DLC] = {0};
    tx_data[0] = (uint8_t) (data->adc_voltage & 0xFF);
    tx_data[1] = (uint8_t) ((data->adc_voltage >> 8) & 0xFF);
    
    /* Current Data: int32_t */
    tx_data[2] = (uint8_t) (data->current_data & 0xFF);
    tx_data[3] = (uint8_t) ((data->current_data >> 8) & 0xFF);
    tx_data[4] = (uint8_t) ((data->current_data >> 16) & 0xFF);
    tx_data[5] = (uint8_t) ((data->current_data >> 24) & 0xFF);

    // Send over CAN
    if (can_send(hcan1, &tx_header, tx_data, ticksToWait) != CAN_OK) {
        return AMPERES_CAN_SEND_FAIL;
    }
    
    return AMPERES_OK;
}