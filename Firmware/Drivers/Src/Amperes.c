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
ADC_ChannelConfTypeDef sConfig = {
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
    hcan1->Init.Mode = CAN_MODE_LOOPBACK;   // TODO: change to normal after verification
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
    int32_t adc_signed = ((int32_t)adc_val*1000) - AMPERES_ADC_VREF_SCALED;
    // Convert to mA using intermediate scaling (see comments in Amperes.h)
    int32_t current_mA = (int32_t) (((int64_t) adc_signed * AMPERES_CONV_NUM_SCALED) / ((int64_t)AMPERES_CONV_DEN_SCALED));
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
    tx_header.StdId = AMPERES_CAN_STD_ID;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.IDE = CAN_ID_STD;
    tx_header.DLC = AMPERES_CAN_DLC;
    tx_header.TransmitGlobalTime = DISABLE;

    // Split data into uint8 elements
    // Little Endian: LSB at index 0

    /* Raw ADC Value: uint16_t */
    uint8_t tx_data[AMPERES_CAN_DLC] = {0};
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
        HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN, 1);
        HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_DISCHARGE_PIN, 0);
    } else if (currentValue > 0) {
        // Positive means discharging
        HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_DISCHARGE_PIN, 1);
        HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN, 0);
    }
}


/** ================================================================
 *  ADC, CAN MSP Init
 * ================================================================ */

  /**
  * @brief ADC MSP Initialization.
  * Configures hardware: clock, GPIO, interrupts.
  * Called by HAL_ADC_Init()
  * @param hadc: ADC handle pointer
  * @retval None
  */
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
      Error_Handler();
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

    /* ADC1 interrupt Init */
    HAL_NVIC_SetPriority(ADC1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(ADC1_IRQn);
  }
}

/**
  * @brief ADC MSP De-Initialization
  * This function freeze the hardware resources.
  * @param hadc: ADC handle pointer
  * @retval None
  */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc) {
  if(hadc->Instance==ADC1)   {
    /* Peripheral clock disable */
    __HAL_RCC_ADC_CLK_DISABLE();

    /**ADC1 GPIO Configuration
    PA0     ------> ADC1_IN5
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0);

    /* ADC1 interrupt DeInit */
    HAL_NVIC_DisableIRQ(ADC1_IRQn);
  }
}

/**
  * @brief CAN MSP Initialization
  * Configures hardware: clock, GPIO, interrupts
  * @param hcan: CAN handle pointer
  * @retval None
  */
void HAL_CAN_MspInit(CAN_HandleTypeDef* hcan) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(hcan->Instance==CAN1)   {
    /* Peripheral clock enable */
    __HAL_RCC_CAN1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**CAN1 GPIO Configuration
    PB8     ------> CAN1_RX
    PB9     ------> CAN1_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* CAN1 interrupt Init */
    HAL_NVIC_SetPriority(CAN1_TX_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
  }
}

/**
  * @brief CAN MSP De-Initialization
  * This function freeze the hardware resources.
  * @param hcan: CAN handle pointer
  * @retval None
  */
void HAL_CAN_MspDeInit(CAN_HandleTypeDef* hcan) {
  if(hcan->Instance==CAN1) {
    /* Peripheral clock disable */
    __HAL_RCC_CAN1_CLK_DISABLE();

    /**CAN1 GPIO Configuration
    PB8     ------> CAN1_RX
    PB9     ------> CAN1_TX
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8|GPIO_PIN_9);

    /* CAN1 interrupt DeInit */
    HAL_NVIC_DisableIRQ(CAN1_TX_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
  }
}