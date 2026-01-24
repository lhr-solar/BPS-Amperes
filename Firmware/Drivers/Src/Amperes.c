#include "Amperes.h"

/** ================================================================
 *  Local Variables
 * ================================================================ */
/* ADC Queue */
#ifndef ADC_QUEUE_LENGTH
    // TODO: why size
    #define ADC_QUEUE_LENGTH 100    
#endif
#define ITEM_SIZE sizeof(uint32_t)
QueueHandle_t adc_queue;
uint8_t qStorage[ADC_QUEUE_LENGTH * ITEM_SIZE];
static StaticQueue_t xStaticQueue;

/* ADC Timer */
TIM_HandleTypeDef htim6;


/** ================================================================
 *  Local Init Functions
 * ================================================================ */
/**
 * @brief Override MSP GPIO init
 */
void HAL_ADC_MspGPIOInit(ADC_HandleTypeDef* hadc) {
    // Initialize PA0 for ADC
    GPIO_InitTypeDef input =  {
        .Mode = GPIO_MODE_ANALOG,
        .Pull = GPIO_NOPULL, 
        .Pin = AMPERES_ADC_PIN
    };
    HAL_GPIO_Init(AMPERES_ADC_PORT, &input);

    // Initialize ports for LED debug
    GPIO_InitTypeDef led_config = {
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Pin = AMPERES_HB_PIN | AMPERES_FAULT_PIN | AMPERES_CHARGE_PIN | AMPERES_DISCHARGE_PIN
    };
    HAL_GPIO_Init(AMPERES_LED_PORT, &led_config);

    __HAL_RCC_GPIOA_CLK_ENABLE();
}

static bool MX_TIM6_Init(void) {
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  // 100 Hz i think or something
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 7999;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 99;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK) {
    return false;
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK) {
    return false;
  }
  return true;
}

static bool Amperes_ADC_Init() {
    /* ================ Hardware Timer 6 Init ================ */
    if (!MX_TIM6_Init()) return false;

    /* ================ ADC Init ================ */
    /* Initialize queue */
    adc_queue = xQueueCreateStatic(ADC_QUEUE_LENGTH, ITEM_SIZE, qStorage, &xStaticQueue);

    /* ADC Init struct*/
    ADC_InitTypeDef init = {0};
    init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2; /* ADC clock: TODO sync or async */
    // init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV2;
    init.Resolution = ADC_RESOLUTION_12B; 
    init.DataAlign = ADC_DATAALIGN_RIGHT;
    init.ScanConvMode = ADC_SCAN_DISABLE;
    init.EOCSelection = ADC_EOC_SINGLE_CONV;
    init.LowPowerAutoWait = DISABLE;
    init.ContinuousConvMode = DISABLE;  // Single Conversion
    init.NbrOfConversion = 1;
    init.DiscontinuousConvMode = DISABLE;

    /* Software triggered conversion */
    // init.ExternalTrigConv = ADC_SOFTWARE_START;
    // init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;

    /* Timer (TIM6) triggered conversion */
    init.ExternalTrigConv = ADC_EXTERNALTRIG_T6_TRGO;
    init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;

    init.DMAContinuousRequests = DISABLE;
    init.Overrun = ADC_OVR_DATA_PRESERVED;
    init.OversamplingMode = DISABLE;

    /* Initialize ADC */
    volatile adc_status_t s = adc_init(&init, hadc1);
    s+=0;
    if (s != ADC_OK) return false;
    
    /* Calibrate after initialization (must be after clock setup)*/
    HAL_ADCEx_Calibration_Start(hadc1, ADC_SINGLE_ENDED);
    
    /* Start ADC */
    if (adc_read(AMPERES_ADC_CHANNEL, AMPERES_SAMPLE_TIME, hadc1, &adc_queue) != ADC_OK) {
        return false;
    }

    /* Start Timer (after ADC)*/
    HAL_TIM_Base_Start(&htim6);

    return true;
}

// static bool Amperes_CAN_Init() {
//     // Create filter
//     CAN_FilterTypeDef  sFilterConfig;
//     sFilterConfig.FilterBank = 0;
//     sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
//     sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
//     sFilterConfig.FilterIdHigh = 0x0000;
//     sFilterConfig.FilterIdLow = 0x0000;
//     sFilterConfig.FilterMaskIdHigh = 0x0000;
//     sFilterConfig.FilterMaskIdLow = 0x0000;
//     sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
//     sFilterConfig.FilterActivation = ENABLE;
//     sFilterConfig.SlaveStartFilterBank = 14;

//     // setup can1 init
//     hcan1->Init.Prescaler = 5;
//     hcan1->Init.Mode = CAN_MODE_LOOPBACK;   // TODO: CHANGE LATER
//     hcan1->Init.SyncJumpWidth = CAN_SJW_1TQ;
//     hcan1->Init.TimeSeg1 = CAN_BS1_6TQ;
//     hcan1->Init.TimeSeg2 = CAN_BS2_2TQ;
//     hcan1->Init.TimeTriggeredMode = DISABLE;
//     hcan1->Init.AutoBusOff = DISABLE;
//     hcan1->Init.AutoWakeUp = DISABLE;
//     hcan1->Init.AutoRetransmission = ENABLE;
//     hcan1->Init.ReceiveFifoLocked = DISABLE;

//     // If TransmitFifoPriority is disabled, the hardware selects the mailbox based on the message ID priority. 
//     // If enabled, the hardware uses a FIFO mechanism to select the mailbox based on the order of transmission requests.
//     hcan1->Init.TransmitFifoPriority = ENABLE;

//     // initialize CAN1
//     if (can_init(hcan1, &sFilterConfig) != CAN_OK) return false;    // or call error handler?

//     // Debug
//     #ifdef DEBUG

//     #endif
    
//     return true;
// }


/** ================================================================
 *  Amperes Functions
 * ================================================================ */

AmperesStatus_t Amperes_Init() {
    // Init HAL?
    // if (HAL_Init() != HAL_OK) return AMPERES_INIT_FAIL;

    // Init ADC
    if (!Amperes_ADC_Init()) return AMPERES_INIT_FAIL;

    // Init CAN
    // if (!Amperes_CAN_Init()) return AMPERES_INIT_FAIL;

    // Start CAN
    // if (can_start(hcan1) != CAN_OK) return AMPERES_INIT_FAIL;

    return AMPERES_OK;
}


AmperesStatus_t Amperes_GetReading(int32_t *current_reading) {
    // Read from ADC into queue
    // if (adc_read(AMPERES_ADC_CHANNEL, AMPERES_SAMPLE_TIME, hadc1, &adc_queue) != ADC_OK) {
    //     return AMPERES_ADC_FAIL;
    // }

    // Read ADC value from queue
    int32_t adc_value = 0;
    if (xQueueReceive(adc_queue, &adc_value, 0) == pdPASS) {
        *current_reading = (int8_t) AMPERES_ADC_TO_CURRENT(adc_value);
        return AMPERES_OK;
    } else {
        return AMPERES_QUEUE_FULL;
    }
}


AmperesStatus_t Amperes_SendCAN(int32_t data) {
    // Create CAN payload
    CAN_TxHeaderTypeDef tx_header = {0};
    tx_header.StdId = AMPERES_STD_ID;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.IDE = CAN_ID_STD;
    tx_header.DLC = 2;
    tx_header.TransmitGlobalTime = DISABLE;

    // Split int32 data into 4 uint8 elements; LSB at index 0
    uint8_t tx_data[8] = {0};
    tx_data[0] = (uint8_t) (data & 0xFF);
    tx_data[1] = (uint8_t) ((data >> 8) & 0xFF);
    tx_data[2] = (uint8_t) ((data >> 16) & 0xFF);
    tx_data[3] = (uint8_t) ((data >> 24) & 0xFF);

    // Send over CAN
    if (can_send(hcan1, &tx_header, tx_data, portMAX_DELAY) != CAN_SENT) {
        return AMPERES_CAN_SEND_FAIL;
    }
    
    return AMPERES_OK;
}
