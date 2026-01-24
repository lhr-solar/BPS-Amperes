#include "stm32xx_hal.h"
#include "Amperes.h"


StaticTask_t xTaskBuffer;
StackType_t xStack[ 200 ];

void Task_ReadADC(void *pvParameters) {
    int32_t reading = 0;

    while(1) {
        AmperesStatus_t stat = Amperes_GetReading(&reading);
        if (stat != AMPERES_OK) error_handler();

        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_HB_PIN);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main() {
    HAL_Init();
    SystemClock_Config();
    
    // Init LED
    GPIO_InitTypeDef led_config = {
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Pin = GPIO_PIN_5
    };
    __HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_GPIO_Init(GPIOA, &led_config);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 1);

    if(Amperes_Init(false) == AMPERES_INIT_FAIL) error_handler();

    xTaskCreateStatic(Task_ReadADC,
                    "ADC Test",
                    configMINIMAL_STACK_SIZE,
                    (void*) 1,
                    tskIDLE_PRIORITY+5,
                    xStack,
                    &xTaskBuffer);

    vTaskStartScheduler();

    return 0;
}