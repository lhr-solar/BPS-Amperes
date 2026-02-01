#include "stm32xx_hal.h"
#include "Amperes.h"
#include "Tasks.h"

StaticTask_t xADCTaskBuffer;
StackType_t xADCStack[ 200 ];
StaticTask_t xBlinkyTaskBuffer;
StackType_t xBlinkyStack[ 200 ];

void Task_ReadADC(void *pvParameters) {
    int32_t reading = 0;

    while(1) {
        // Manually polling ADC (not timer driven)
        AmperesStatus_t stat = Amperes_GetReading(&reading);
        while (stat != AMPERES_OK) error_handler();
        
        // // using current value: approx -50000 to 82000 mA
        // if (reading < 0) reading *= -1;
        // reading /= 50;

        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN);
        vTaskDelay(pdMS_TO_TICKS(reading+100));
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

    if(Amperes_Init(false) == AMPERES_INIT_FAIL) error_handler();

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