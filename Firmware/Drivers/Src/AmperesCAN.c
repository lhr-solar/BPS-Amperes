#include "AmperesCAN.h"
#include <string.h>

/** ================================================================
 *  Queue for CAN Message mirroring
 * ================================================================ */

QueueHandle_t can_tx_queue;
uint8_t can_tx_qStorage[CAN_TX_QUEUE_LENGTH * CAN_TX_ITEM_SIZE];
static StaticQueue_t xStaticQueue_can_tx;


 /** ================================================================
 *  Local Init Function
 * ================================================================ */

/**
 * @brief Initializes CAN filter and hardware.
 * - Fails if can_init() fails.
 * - Called in Amperes_Init().
 */
static bool MX_CAN_Init() {
    /* Initialize queue */
    can_tx_queue = xQueueCreateStatic(
        CAN_TX_QUEUE_LENGTH, 
        CAN_TX_ITEM_SIZE, 
        can_tx_qStorage, 
        &xStaticQueue_can_tx
    );
    if (can_tx_queue == NULL) return false;

    /* Create CAN filter */
    /* For testing: accept all incoming IDs */
    CAN_FilterTypeDef  sFilterConfig;
    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    #ifdef TEST_MODE
    /* For testing: accept all IDs */
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    #else
    /* For production: reject all IDs */
    sFilterConfig.FilterIdHigh = 0xFFFF;
    sFilterConfig.FilterIdLow = 0xFFFF;
    sFilterConfig.FilterMaskIdHigh = 0xFFFF;
    sFilterConfig.FilterMaskIdLow = 0xFFFF;
    #endif
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14;

    /* CAN1 Init Struct */
    // Baud rate is 250 kbit/s
    hcan1->Instance = CAN1;
    hcan1->Init.Prescaler = 20;
    hcan1->Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan1->Init.TimeSeg1 = CAN_BS1_13TQ;
    hcan1->Init.TimeSeg2 = CAN_BS2_2TQ;
    #ifdef TEST_MODE
    /* For testing: loopback mode */
    hcan1->Init.Mode = CAN_MODE_LOOPBACK;
    #else
    /* For production: normal CAN mode */
    hcan1->Init.Mode = CAN_MODE_NORMAL;
    #endif
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

AmperesStatus_t Amperes_SendCAN(bps_pack_current_t *data, TickType_t ticksToWait) {
    // Create CAN payload
    CAN_TxHeaderTypeDef tx_header = {0};
    tx_header.StdId = AMPERES_MSG_ID;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.IDE = CAN_ID_STD;
    tx_header.DLC = AMPERES_MSG_DLC;
    tx_header.TransmitGlobalTime = DISABLE;

    // Convert ADC to voltage
    uint16_t adc_to_voltage = (((uint32_t) data->Main_Battery_Current_RawV * 3300)/4095);

    // Pack data into fields: reference bps_pack_current_t struct
    // Little Endian (LSB first): e.g. 0x123456 is stored as [56][34][12]
    uint8_t tx_data[AMPERES_MSG_DLC] = {0};

    /* Current Data: int32_t into 24 bit field (little endian)*/
    memcpy(tx_data, &data->Main_Battery_Current, 3);

    /* Raw Voltage Value: uint16_t into 16 bit field (little endian)*/
    memcpy(tx_data+3, &adc_to_voltage, 2);

    // Send over CAN
    if (can_send(hcan1, &tx_header, tx_data, ticksToWait) != CAN_OK) {
        return AMPERES_CAN_SEND_FAIL;
    }
    
    return AMPERES_OK;
}