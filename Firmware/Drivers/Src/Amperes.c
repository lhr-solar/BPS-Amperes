#include "Amperes.h"

/** ================================================================
 *  Local Variables
 * ================================================================ */
/** ADC Queue to store ADC conversion
 * - Only needs to hold 1 element from conversion
 */
#define ADC_ITEM_SIZE sizeof(int32_t)
#ifndef ADC_QUEUE_LENGTH
    #define ADC_QUEUE_LENGTH 2
#endif
QueueHandle_t adc_queue;
uint8_t adc_qStorage[ADC_QUEUE_LENGTH * ADC_ITEM_SIZE];
static StaticQueue_t xStaticQueue_adc;

/* CAN Queue (to send to CAN)*/
#define CAN_ITEM_SIZE sizeof(AmperesMsg_t)
#ifndef CAN_QUEUE_LENGTH
// TODO: PICK SIZE
#define CAN_QUEUE_LENGTH 20
#endif
QueueHandle_t can_queue;
uint8_t can_qStorage[CAN_QUEUE_LENGTH * CAN_ITEM_SIZE];
static StaticQueue_t xStaticQueue_can;


/** ================================================================
 *  Local Init Functions
 * ================================================================ */

 void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    if(hadc->Instance==ADC1) {
    /** Initializes the peripherals clock
     */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_PLLSAI1;
    PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_MSI;
    PeriphClkInit.PLLSAI1.PLLSAI1M = 1;
    PeriphClkInit.PLLSAI1.PLLSAI1N = 16;
    PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV7;
    PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV2;
    PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV2;
    PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_ADC1CLK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
      error_handler();
    }

    /* Peripheral clock enable */
    __HAL_RCC_ADC_CLK_ENABLE();

    /**ADC1 GPIO Configuration
    PA0     ------> ADC1_IN5
    */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = AMPERES_ADC_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(AMPERES_ADC_PORT, &GPIO_InitStruct);

    /* ADC1 interrupt Init: PRIO MUST BE AT LEAST 5 */
    HAL_NVIC_SetPriority(ADC1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(ADC1_IRQn);
  }

}

static bool Amperes_ADC_Init() {
    /* Initialize ADC pin, clock, and interrupt */
    HAL_ADC_MspInit(hadc1);

    /* Initialize queue */
    adc_queue = xQueueCreateStatic(
        ADC_QUEUE_LENGTH, 
        ADC_ITEM_SIZE, 
        adc_qStorage, 
        &xStaticQueue_adc
    );
    
    /* ================ ADC Init Struct ================ */
    ADC_InitTypeDef init = {0};

    init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2; /* ADC clock: synchronous */
    init.Resolution = ADC_RESOLUTION_12B;   /* 12 bit ADC */
    init.DataAlign = ADC_DATAALIGN_RIGHT;
    init.ScanConvMode = ADC_SCAN_DISABLE;
    init.EOCSelection = ADC_EOC_SINGLE_CONV;
    init.LowPowerAutoWait = DISABLE;
    init.ContinuousConvMode = DISABLE;  /* Single Conversion */
    init.NbrOfConversion = 1;
    init.DiscontinuousConvMode = DISABLE;
    init.DMAContinuousRequests = DISABLE;
    init.Overrun = ADC_OVR_DATA_PRESERVED;
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

static bool Amperes_CAN_Init() {
    /* Initialize CAN queue */
    can_queue = xQueueCreateStatic(
        CAN_QUEUE_LENGTH, 
        CAN_ITEM_SIZE, 
        can_qStorage, 
        &xStaticQueue_can
    );

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
    hcan1->Init.Prescaler = 5;
    hcan1->Init.Mode = CAN_MODE_LOOPBACK;   // TODO: CHANGE LATER TO NO LOOPBACK
    hcan1->Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan1->Init.TimeSeg1 = CAN_BS1_6TQ;
    hcan1->Init.TimeSeg2 = CAN_BS2_2TQ;
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
    // Get signed ADC value in terms of reference point; scale for fixed point math
    int32_t adc_signed = (int32_t)(adc_val*1000) - AMPERES_ADC_VREF_SCALED;
    // Convert to mA using intermediate scaling (see comments in Amperes.h)
    int32_t current_mA = ((int64_t)(adc_signed * AMPERES_CONV_NUM_SCALED)) / (AMPERES_CONV_DEN_SCALED);
    return current_mA;
}

AmperesStatus_t Amperes_StartADC(bool clearQueue) {
    // Clear queue (e.g. if blocking in GetReading based on queue empty -> value)
    if (clearQueue) { xQueueReset(adc_queue); }
    // Start ADC conversion: result will appear in queue
    if (adc_read(AMPERES_ADC_CHANNEL, AMPERES_SAMPLE_TIME, hadc1, adc_queue) != ADC_OK) {
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


AmperesStatus_t Amperes_SendCAN(AmperesMsg_t *data) {
    // Create CAN payload
    CAN_TxHeaderTypeDef tx_header = {0};
    tx_header.StdId = AMPERES_STD_ID;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.IDE = CAN_ID_STD;
    tx_header.DLC = 8;
    tx_header.TransmitGlobalTime = DISABLE;

    // Split int32 data into 4 uint8 elements; LSB at index 0

    /* Raw ADC Value */
    uint8_t tx_data[8] = {0};
    tx_data[0] = (uint8_t) (data->adc_voltage & 0xFF);
    tx_data[1] = (uint8_t) ((data->adc_voltage >> 8) & 0xFF);
    tx_data[2] = (uint8_t) ((data->adc_voltage >> 16) & 0xFF);
    tx_data[3] = (uint8_t) ((data->adc_voltage >> 24) & 0xFF);
    
    /* Current Data */
    tx_data[4] = (uint8_t) (data->current_data & 0xFF);
    tx_data[5] = (uint8_t) ((data->current_data >> 8) & 0xFF);
    tx_data[6] = (uint8_t) ((data->current_data >> 16) & 0xFF);
    tx_data[7] = (uint8_t) ((data->current_data >> 24) & 0xFF);

    // Send over CAN
    if (can_send(hcan1, &tx_header, tx_data, portMAX_DELAY) != CAN_SENT) {
        return AMPERES_CAN_SEND_FAIL;
    }
    
    return AMPERES_OK;
}
