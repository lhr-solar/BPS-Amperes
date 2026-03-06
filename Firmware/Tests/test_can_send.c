#include "AmperesConfig.h"
#include "AmperesCAN.h"
#include "common.h"

/** ================================================================
 * CAN Test
 * - Sends Amperes CAN messages.
 * - Debug w/ printf and a CAN adapter.
 * ================================================================ */

StaticTask_t xTaskBuffer;
StackType_t xStack[200];

void Task_SendCAN() {
    // Transmit
    AmperesMsg_t payload = {
        .adc_voltage = 4095,
        .current_data = 80000
    };
    
    while (1) {
        // Increment data
        if (payload.adc_voltage < 15) {
            payload.adc_voltage = 4095;
        } else {
            payload.adc_voltage -= 15;
        }
        if (payload.current_data == -30000) {
            payload.current_data = 80000;
        } else {
            payload.current_data -= 1000;
        }

        // Send CAN data
        if (Amperes_SendCAN(&payload, portMAX_DELAY) != AMPERES_OK) {
            printf("no can send \r\n");
            Error_Handler();
        }
        printf("\r\nSENT \t adc %4d | current %5li \r\n", payload.adc_voltage, payload.current_data);

        // Blinky for verification that we are still in loop
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_HB_PIN);

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

int main() {
    HAL_Init();
    SystemClock_Config();
    UART_Printf_Init();

    MX_GPIO_Init();
    if (Amperes_CAN_Init() != AMPERES_OK) Error_Handler();
    if (Amperes_CAN_Start() != AMPERES_OK) Error_Handler();

    xTaskCreateStatic(Task_SendCAN,
                    "CAN Test",
                    200,
                    (void*) 1,
                    tskIDLE_PRIORITY+5,
                    xStack,
                    &xTaskBuffer);

    vTaskStartScheduler();

    return 0;
}