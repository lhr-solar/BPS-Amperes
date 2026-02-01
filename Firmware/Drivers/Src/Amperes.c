#include "Amperes.h"

/** ================================================================
 *  Local Variables
 * ================================================================ */
/* ADC Queue (to store adc conversion)*/
#define ITEM_SIZE sizeof(int32_t)
#ifndef ADC_QUEUE_LENGTH
    // TODO: PICK SIZE
    #define ADC_QUEUE_LENGTH 100    
#endif

QueueHandle_t adc_queue;
uint8_t adc_qStorage[ADC_QUEUE_LENGTH * ITEM_SIZE];
static StaticQueue_t xStaticQueue_adc;

/* CAN Queue (to send to CAN)*/
#ifndef CAN_QUEUE_LENGTH
    // TODO: PICK SIZE
    #define CAN_QUEUE_LENGTH 100    
#endif

QueueHandle_t can_queue;
uint8_t can_qStorage[CAN_QUEUE_LENGTH * ITEM_SIZE];
static StaticQueue_t xStaticQueue_can;

/* ADC Timer */
TIM_HandleTypeDef htim6;


/** ================================================================
 *  Local Init Functions
 * ================================================================ */
/**
 * @brief Override MSP GPIO init
 */
void HAL_ADC_MspGPIOInit() { // param: ADC_HandleTypeDef* hadc ??
    // Initialize PA0 for ADC
    GPIO_InitTypeDef input =  {
        .Mode = GPIO_MODE_ANALOG_ADC_CONTROL,
        .Pull = GPIO_NOPULL, 
        .Pin = AMPERES_ADC_PIN
    };
    HAL_GPIO_Init(AMPERES_ADC_PORT, &input);
    __HAL_RCC_GPIOA_CLK_ENABLE();
}

static bool MX_TIM6_Init(void) {
    /**
     * Timer init
     * 1 KHz
     * Rate = (80 MHz) / ((Prescaler + 1)*(Period + 1))
     */
    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 79;
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.Period = 999;
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim6) != HAL_OK) {
        return false;
    }

    /* Enable interrupt */
    // HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 5, 0);
    // HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);

    /* TRGO Trigger */
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE; // update event as trigger
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK) {
        return false;
    }

    /* Enable timer 6 clock*/
    __HAL_RCC_TIM6_CLK_ENABLE();
    return true;
}

static bool Amperes_ADC_Init(bool timerDriven) {
    /* ================ Hardware Timer 6 Init ================ */
    if (timerDriven) {
        if (!MX_TIM6_Init()) return false;
    }

    /* ================ ADC Init ================ */
    /* Initialize queue */
    adc_queue = xQueueCreateStatic(ADC_QUEUE_LENGTH, ITEM_SIZE, adc_qStorage, &xStaticQueue_adc);

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

    if (timerDriven) {
        /* Timer (TIM6) triggered conversion */
        init.ExternalTrigConv = ADC_EXTERNALTRIG_T6_TRGO;
        init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    } else {
        /* Software triggered conversion */
        init.ExternalTrigConv = ADC_SOFTWARE_START;
        init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    }

    init.DMAContinuousRequests = DISABLE;
    init.Overrun = ADC_OVR_DATA_PRESERVED;
    init.OversamplingMode = DISABLE;

    /* Initialize ADC */
    volatile adc_status_t s = adc_init(&init, hadc1);
    s+=0;
    if (s != ADC_OK) return false;
    
    /* Calibrate after initialization (must be after clock setup)*/
    HAL_ADCEx_Calibration_Start(hadc1, ADC_SINGLE_ENDED);

    if (timerDriven) {
        /* Start ADC: Set channel, enable interrupts */
        if (adc_read(AMPERES_ADC_CHANNEL, AMPERES_SAMPLE_TIME, hadc1, &adc_queue) != ADC_OK) {
            return false;
        }
        /* Start Timer (after ADC)*/
        HAL_TIM_Base_Start(&htim6);
    }

    return true;
}

static bool Amperes_CAN_Init() {
    /* Initialize CAN queue */
    can_queue = xQueueCreateStatic(CAN_QUEUE_LENGTH, ITEM_SIZE, can_qStorage, &xStaticQueue_can);

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
    hcan1->Init.Mode = CAN_MODE_LOOPBACK;   // TODO: CHANGE LATER
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
    if (can_init(hcan1, &sFilterConfig) != CAN_OK) return false;    // or call error handler?

    // Debug
    #ifdef DEBUG
    #endif
    
    return true;
}


/** ================================================================
 *  Amperes Functions
 * ================================================================ */

AmperesStatus_t Amperes_Init(bool timerDriven) {
    /* HAL_Init should be run before this is called */

    // Initialize GPIO
    MX_GPIO_Init();

    // Init ADC
    if (!Amperes_ADC_Init(timerDriven)) return AMPERES_INIT_FAIL;

    // Init CAN
    if (!Amperes_CAN_Init()) return AMPERES_INIT_FAIL;

    // Start CAN
    if (can_start(hcan1) != CAN_OK) return AMPERES_INIT_FAIL;

    return AMPERES_OK;
}

int32_t Amperes_ADCToCurrent(uint16_t reading) {
    // TODO: fixed point conversion (maybe to uA?)
    int32_t adc_to_mV = reading * (3300/4095);
    adc_to_mV -= AMPERES_mVREF;
    adc_to_mV *= 40;    // 1 / ((250 E-6 ohm)*(100 V/V))
    return adc_to_mV;
}

AmperesStatus_t Amperes_StartADC() {
    // Read from ADC into queue
    if (adc_read(AMPERES_ADC_CHANNEL, AMPERES_SAMPLE_TIME, hadc1, &adc_queue) != ADC_OK) {
        return AMPERES_ADC_START_FAIL;
    }
    return AMPERES_OK;
}

AmperesStatus_t Amperes_GetReading(AmperesMsg_t *message) {
    // Block until ADC value is in queue
    if (xQueueReceive(adc_queue, &(message->adc_voltage), portMAX_DELAY) != pdPASS) { 
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
