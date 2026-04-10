#include "AmperesConfig.h"
#include "Tasks.h"

/** ================================================================
 * Amperes Test
 * - Tests entire system: ADC, CAN
 * ================================================================ */

StaticTask_t xBlinkyTaskBuffer;
StackType_t xBlinkyStack[configMINIMAL_STACK_SIZE];

void Task_Blinky(void *pvParameters) {
    while (1) {
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_HB_PIN);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int main() {
    HAL_Init();
    SystemClock_Config();
    
    // Amperes Task
    xTaskCreateStatic(
        Task_Init,           
        "Initialize Tasks",         
        TASK_INIT_STACK_SIZE,
        (void*)NULL,         
        TASK_INIT_PRIO,      
        Task_Init_Stack,     
        &Task_Init_Buffer    
    );

    // xTaskCreateStatic(
    //     Task_Blinky,
    //     "Blinky",
    //     configMINIMAL_STACK_SIZE,
    //     (void*) NULL,
    //     tskIDLE_PRIORITY+2,
    //     xBlinkyStack,
    //     &xBlinkyTaskBuffer
    // );

    // Start scheduler
    vTaskStartScheduler();

    // Code should not reach here
    Error_Handler();
    while(1) {}

    return 0;
}
