#include "Amperes.h"

/** ================================================================
 *  Local Variables
 * ================================================================ */

/** ADC Queue to store ADC conversion
 * - Only needs to hold 1 element. Basically a mailbox.
 */
#define ADC_ITEM_SIZE sizeof(uint16_t)
#ifndef ADC_QUEUE_LENGTH
    #define ADC_QUEUE_LENGTH 2
#endif
QueueHandle_t adc_queue;
uint8_t adc_qStorage[ADC_QUEUE_LENGTH * ADC_ITEM_SIZE];
static StaticQueue_t xStaticQueue_adc;

// ADC Config for PA0
static ADC_ChannelConfTypeDef sConfig = {
    .Channel = AMPERES_ADC_CHANNEL,
    .Rank = ADC_REGULAR_RANK_1,
    .SamplingTime = AMPERES_SAMPLE_TIME,
    .SingleDiff = ADC_SINGLE_ENDED,
    .OffsetNumber = ADC_OFFSET_NONE,
    .Offset = 0
};


/** ================================================================
 *  Local Init Functions
 * ================================================================ */

/**
 * @brief Initializes ADC queue and hardware.
 * - Fails if adc_init() fails.
 * - Called by Amperes_Init().
 */
static bool Amperes_ADC_Init() {
    /* Initialize queue */
    adc_queue = xQueueCreateStatic(
        ADC_QUEUE_LENGTH, 
        ADC_ITEM_SIZE, 
        adc_qStorage, 
        &xStaticQueue_adc
    );
    if (adc_queue == NULL) return false;
    
    /* ================ ADC Init Struct ================ */
    ADC_InitTypeDef init = {0};

    init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2; /* ADC clock: synchronous */
    init.Resolution = ADC_RESOLUTION_12B;           /* 12 bit ADC */
    init.DataAlign = ADC_DATAALIGN_RIGHT;
    init.ScanConvMode = ADC_SCAN_DISABLE;
    init.EOCSelection = ADC_EOC_SINGLE_CONV;
    init.LowPowerAutoWait = DISABLE;
    init.ContinuousConvMode = DISABLE;              /* Single Conversion */
    init.NbrOfConversion = 1;
    init.DiscontinuousConvMode = DISABLE;
    init.DMAContinuousRequests = DISABLE;
    init.Overrun = ADC_OVR_DATA_OVERWRITTEN;    // Overwrites data on overrun: vs ADC_OVR_DATA_PRESERVED
    init.OversamplingMode = DISABLE;

    /* Software triggered conversion */
    init.ExternalTrigConv = ADC_SOFTWARE_START;
    init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;

    /* Initialize ADC */
    volatile adc_status_t s = adc_init(&init, hadc1);
    s+=0;
    if (s != ADC_OK) return false;
    
    /* Calibrate after initialization (must be after clock setup)*/
    HAL_ADCEx_Calibration_Start(hadc1, ADC_SINGLE_ENDED);

    return true;
}

/**
 * @brief Initializes CAN filter and hardware.
 * - Fails if can_init() fails.
 * - Called in Amperes_Init().
 */
static bool Amperes_CAN_Init() {
    /* Create CAN filter */
    CAN_FilterTypeDef  sFilterConfig;
    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14;

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
 *  Amperes Functions
 * ================================================================ */

AmperesStatus_t Amperes_Init() {
    /* HAL_Init should be run before this is called */

    // Initialize GPIO
    MX_GPIO_Init();

    // Init ADC
    if (!Amperes_ADC_Init()) return AMPERES_INIT_FAIL;

    // Init CAN
    if (!Amperes_CAN_Init()) return AMPERES_INIT_FAIL;

    // Start CAN
    if (can_start(hcan1) != CAN_OK) return AMPERES_INIT_FAIL;

    return AMPERES_OK;
}

int32_t Amperes_ADCToCurrent(uint16_t adc_val) {
    int64_t current_uA = ((int64_t) adc_val - ADC_0A) * SCALE_CONST;
    int32_t current_mA = (int32_t) (current_uA / 1000);
    return current_mA;
}

AmperesStatus_t Amperes_StartADC(bool clearQueue) {
    // Clear queue if requested
    if (clearQueue) { xQueueReset(adc_queue); }
    // Start ADC conversion: result will appear in queue
    if (adc_read(hadc1, &sConfig, adc_queue) != ADC_OK) {
        return AMPERES_ADC_START_FAIL;
    }
    return AMPERES_OK;
}

AmperesStatus_t Amperes_GetReading(AmperesMsg_t *message, TickType_t ticksToWait) {
    // Get ADC value from queue
    if (xQueueReceive(adc_queue, &(message->adc_voltage), ticksToWait) != pdPASS) { 
        return AMPERES_ADC_READ_FAIL;
    }
    message->current_data = Amperes_ADCToCurrent(message->adc_voltage);
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
    if (can_send(hcan1, &tx_header, tx_data, ticksToWait) != CAN_SENT) {
        return AMPERES_CAN_SEND_FAIL;
    }
    
    return AMPERES_OK;
}

void Amperes_UpdateLEDs(int32_t currentValue) {
    if (currentValue < 0) {
        // Negative means charging
        HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_DISCHARGE_PIN, GPIO_PIN_RESET);
    } else {
        // Positive means discharging
        HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_DISCHARGE_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN, GPIO_PIN_RESET);
    }
}
