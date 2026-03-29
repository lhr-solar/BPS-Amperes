#include "AmperesConfig.h"
#include "Tasks.h"

/** ================================================================
 * ADC Test
 * - Reads from the ADC and prints via printf
 * ================================================================ */

StaticTask_t xBlinkyTaskBuffer;
StackType_t xBlinkyStack[ 200 ];
StaticTask_t xADCTaskBuffer;
StackType_t xADCStack[ 200 ];

void ADC_Task(void *pvParameters) {
    bps_pack_current_t message = {
        .Main_Battery_Current = 0,
        .Main_Battery_Current_RawV = 0
    };
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        // Start ADC reading; reset queue
        if (Amperes_StartADC(true) != AMPERES_OK) {
            Error_Handler();
        };

        // Block (indefinitely) until we receive data in queue
        if (Amperes_GetReading(&message, portMAX_DELAY) != AMPERES_OK) {
            Error_Handler();
        }

        // Debug
        printf("\r\n CURRENT: %5li, ADC: %4d \r\n", message.Main_Battery_Current, message.Main_Battery_Current_RawV);
        if (message.Main_Battery_Current < 0) {
            // Negative means charging
            HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_DISCHARGE_PIN, GPIO_PIN_RESET);
        } else {
            // Positive means discharging
            HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_DISCHARGE_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN, GPIO_PIN_RESET);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
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
    MX_GPIO_Init();
    if (Amperes_ADC_Init()) Error_Handler();

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