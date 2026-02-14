#include "stm32xx_hal.h"
#include "Amperes.h"
#include "Tasks.h"


int main() {
    HAL_Init();
    SystemClock_Config();

    // Start Init Task
    xTaskCreateStatic(
        Task_Init,                  /* The function that implements the task. */
        "Task init",                /* Text name for the task. */
        TASK_INIT_STACK_SIZE,       /* The size (in words) of the stack that should be created for the task. */
        (void*)NULL,                /* Paramter passed into the task. */
        TASK_INIT_PRIO,             /* Task Prioriy. */
        Task_Init_Stack,            /* Stack array. */
        &Task_Init_Buffer           /* Buffer for static allocation. */
    );

    // Start scheduler
    vTaskStartScheduler();

    error_handler();
    while(1) {  
        // Code should not reach here
    }

    return 0;
}
