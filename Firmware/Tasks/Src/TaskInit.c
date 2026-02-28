#include "Tasks.h"

StaticTask_t    Amperes_Task_Buffer;
StackType_t     Amperes_Task_Stack[AMPERES_TASK_STACK_SIZE];

StaticTask_t    Task_Init_Buffer;
StackType_t     Task_Init_Stack[TASK_INIT_STACK_SIZE];

void Task_Init() {
    // Initialize Amperes GPIO, ADC, and CAN
    if (Amperes_Init() == AMPERES_INIT_FAIL) Error_Handler();
    
    // Initialize UART for printf
    UART_Printf_Init();
    
    // Init Amperes
    xTaskCreateStatic(
        Amperes_Task,               /* The function that implements the task. */
        "Amperes Task",             /* Text name for the task. */
        AMPERES_TASK_STACK_SIZE,    /* The size (in words) of the stack that should be created for the task. */
        (void*) NULL,               /* Paramter passed into the task. */
        AMPERES_TASK_PRIO,          /* Task Prioriy. */
        Amperes_Task_Stack,         /* Stack array. */
        &Amperes_Task_Buffer        /* Buffer for static allocation. */
    );

   // Delete Init Task
    vTaskDelete(NULL);
}