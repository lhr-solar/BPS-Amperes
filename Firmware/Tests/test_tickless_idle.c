#include "Tasks.h"

/** ================================================================
 * Tickless Idle Hook test
 * - Overrides weakly defined vPortSuppressTicksAndSleep:
 *   turns on fault LED, sleeps, wakes up on interrupt, and 
 *   turns fault LED off.
 * - Fault LED is on whenever we are in idle task. Other LEDs
 *   should still be blinking.
 * 
 * !! ONLY FOR TESTING. Use default vPortSuppressTicksAndSleep !!
 * !! configUSE_TICKLESS_IDLE must be 1 in FreeRTOSConfig.h !!
 * ================================================================ */

void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime ) {
    HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_FAULT_PIN, GPIO_PIN_SET);
    __WFI();   // Wait For Interrupt
    HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_FAULT_PIN, GPIO_PIN_RESET);
}

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

    // Code should not reach here
    Error_Handler();
    while(1) {}
    
    return 0;
}
