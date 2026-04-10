#include "AmperesADC.h"

/** ================================================================
 *  Queue and ADC Channel Config
 * ================================================================ */

/** ADC Queue to store ADC conversion
 * - Only needs to hold 1 element. Basically a mailbox.
 */
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
 *  Local Init Function
 * ================================================================ */

/**
 * @brief Initializes ADC queue and hardware.
 * - Fails if adc_init() fails.
 * - Called by Amperes_Init().
 */
static bool MX_ADC_Init() {
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

/** ================================================================
 *  Amperes Functions
 * ================================================================ */

AmperesStatus_t Amperes_ADC_Init() {
    // Init ADC
    if (!MX_ADC_Init()) return AMPERES_ADC_INIT_FAIL;
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

AmperesStatus_t Amperes_GetReading(uint16_t *adcReading, TickType_t ticksToWait) {

    if(adcReading == NULL){
        return AMPERES_ADC_READ_FAIL;
    }

    // Get ADC value from queue
    if (xQueueReceive(adc_queue, adcReading, ticksToWait) != pdPASS) { 
        return AMPERES_ADC_READ_FAIL;
    }

    // message->Main_Battery_Current = Amperes_ADCToCurrent(message->Main_Battery_Current_RawV);
    return AMPERES_OK;
}
