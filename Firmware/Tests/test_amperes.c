#include "stm32xx_hal.h"
#include "Amperes.h"
#include "Tasks.h"

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

    if(Amperes_Init() == AMPERES_INIT_FAIL) error_handler();
    MX_UART_Init();
    
    // Amperes Task
    xTaskCreateStatic(
        Amperes_Task,           
        "Amperes Task",         
        AMPERES_TASK_STACK_SIZE,
        (void*)NULL,         
        AMPERES_TASK_PRIO,      
        Amperes_Task_Stack,     
        &Amperes_Task_Buffer    
    );

    xTaskCreateStatic(
        Task_Blinky,
        "Blinky",
        configMINIMAL_STACK_SIZE,
        (void*) NULL,
        tskIDLE_PRIORITY+2,
        xBlinkyStack,
        &xBlinkyTaskBuffer
    );

    // Start scheduler
    vTaskStartScheduler();

    // Code should not reach here
    error_handler();
    while(1) {}

    return 0;
}
