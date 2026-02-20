#include "stm32xx_hal.h"
#include "Amperes.h"
#include "Tasks.h"

StaticTask_t xADCTaskBuffer;
StackType_t xADCStack[ 200 ];
StaticTask_t xBlinkyTaskBuffer;
StackType_t xBlinkyStack[ 200 ];

void Task_ReadADC(void *pvParameters) {
    AmperesMsg_t message;

    // Debug with CAN RX TX
    GPIO_InitTypeDef debug_gpio = {
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Pin = GPIO_PIN_8 | GPIO_PIN_9
    };
    __HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_GPIO_Init(GPIOB, &debug_gpio); 

    while(1) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_8);  // toggle CAN RX for debug
        // Clear queue and start ADC
        Amperes_StartADC(true);
        if (Amperes_GetReading(&message, portMAX_DELAY) != AMPERES_OK) {
            Error_Handler();
        }
        
        // using current value: approx -50000 to 82000 mA
        // if (reading < 0) reading *= -1;
        // reading /= 50;

        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN);
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_8);  // toggle CAN RX for debug
        vTaskDelay(pdMS_TO_TICKS(message.adc_voltage + 50));
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_8);  // toggle CAN RX for debug
    }
}

void Task_Blinky(void *pvParameters) {
    while (1) {
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_HB_PIN);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

int main() {
    HAL_Init();
    SystemClock_Config();

    HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_HB_PIN, 1);

    if(Amperes_Init() == AMPERES_INIT_FAIL) Error_Handler();

    xTaskCreateStatic(
        Task_ReadADC,
        "ADC Blinky Test",
        200,
        (void*) 1,
        tskIDLE_PRIORITY+3,
        xADCStack,
        &xADCTaskBuffer
    );

    xTaskCreateStatic(
        Task_Blinky,
        "Blinky",
        200,
        (void*) 1,
        tskIDLE_PRIORITY+2,
        xBlinkyStack,
        &xBlinkyTaskBuffer
    );

    vTaskStartScheduler();

    return 0;
}