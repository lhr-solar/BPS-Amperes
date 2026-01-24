#include "stm32xx_hal.h"
#include "Amperes.h"


StaticTask_t xTaskBuffer;
StackType_t xStack[ 200 ];

void Task_ReadADC(void *pvParameters) {
    // int32_t reading = 0;

    while(1) {
        // Manually polling ADC (not timer driven)

        AmperesStatus_t stat = Amperes_GetReading(&reading);
        while (stat != AMPERES_OK) error_handler();
        
        // // using current value: approx -50 to 82 A
        if (reading < 0) reading *= -1;
        reading *= 10;

        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN);
        vTaskDelay(pdMS_TO_TICKS(reading));
    }
}

int main() {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    
    // Init LED
    // GPIO_InitTypeDef led_config = {
    //     .Mode = GPIO_MODE_OUTPUT_PP,
    //     .Pull = GPIO_NOPULL,
    //     .Pin = AMPERES_HB_PIN | AMPERES_FAULT_PIN | AMPERES_CHARGE_PIN | AMPERES_DISCHARGE_PIN
    // };
    // __HAL_RCC_GPIOA_CLK_ENABLE();
    // HAL_GPIO_Init(AMPERES_GPIO_PORT, &led_config);

    HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_HB_PIN, 1);

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