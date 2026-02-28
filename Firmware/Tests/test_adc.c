#include "stm32xx_hal.h"
#include "Amperes.h"
#include "Tasks.h"

StaticTask_t xBlinkyTaskBuffer;
StackType_t xBlinkyStack[ 200 ];
StaticTask_t xADCTaskBuffer;
StackType_t xADCStack[ 200 ];

void ADC_Task(void *pvParameters) {
    AmperesMsg_t message = {
        .adc_voltage = 0,
        .current_data = 0
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
        printf("\r\n ADC: %4d, CURRENT: %5li \r\n", message.adc_voltage, message.current_data);
        Amperes_UpdateLEDs(message.current_data);

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