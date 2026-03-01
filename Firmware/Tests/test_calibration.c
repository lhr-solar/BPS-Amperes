#include "stm32xx_hal.h"
#include "Amperes.h"
#include "Tasks.h"

/** ================================================================
 * Calibration Test
 * - Samples the ADC 100 times and prints the average.
 * - Pass in a known current to find calibration values and put 
 *   in Amperes.h.
 * ================================================================ */

StaticTask_t xBlinkyTaskBuffer;
StackType_t xBlinkyStack[ 200 ];
StaticTask_t xADCTaskBuffer;
StackType_t xADCStack[ 200 ];

/**
 * Measure 100 ADC readings at known currents and outputs to console.
 */
void ADC_Task(void *pvParameters) {
    AmperesMsg_t message = {0};
    uint32_t adc_sum = 0;
    int64_t current_sum = 0;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (uint8_t i=0; i <100; i++) {
        // Start ADC reading; reset queue
        if (Amperes_StartADC(true) != AMPERES_OK) {
            Error_Handler();
        };

        // Block (indefinitely) until we receive data in queue
        if (Amperes_GetReading(&message, portMAX_DELAY) != AMPERES_OK) {
            Error_Handler();
        }

        // Sum
        adc_sum += message.adc_voltage;
        current_sum += message.current_data;

        // printf("\r\n ADC: %4d, CURRENT: %5li \r\n", message.adc_voltage, message.current_data);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1));
    }
    // Print result
    printf("\r\n AVG --- ADC: %4d, CURRENT: %5li \r\n", (uint16_t) (adc_sum/100), (int32_t) (current_sum/100));
    vTaskDelete(NULL);
}

void Task_Blinky(void *pvParameters) {
    while (1) {
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_HB_PIN);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int main() {
    HAL_Init();
    SystemClock_Config();
    
    // UART for printf
    UART_Printf_Init();

    // Amperes hardware
    if(Amperes_Init() == AMPERES_INIT_FAIL) Error_Handler();

    xTaskCreateStatic(
        ADC_Task,
        "ADC Task",
        200,
        (void*) NULL,
        tskIDLE_PRIORITY + 4,
        xADCStack,
        &xADCTaskBuffer
    );

    xTaskCreateStatic(
        Task_Blinky,
        "Blinky",
        200,
        (void*) NULL,
        tskIDLE_PRIORITY+3,
        xBlinkyStack,
        &xBlinkyTaskBuffer
    );

    vTaskStartScheduler();

    return 0;
}