#include "Tasks.h"

StaticTask_t    Amperes_Task_Buffer;
StackType_t     Amperes_Task_Stack[AMPERES_TASK_STACK_SIZE];

StaticTask_t    Task_Init_Buffer;
StackType_t     Task_Init_Stack[TASK_INIT_STACK_SIZE];

void Task_Init() {
    // Init Amperes GPIO, ADC, and CAN
    if (Amperes_Init() == AMPERES_INIT_FAIL) error_handler();
    
    // Init Amperes
    xTaskCreateStatic(
        Amperes_Task,
        "Amperes Task",
        AMPERES_TASK_STACK_SIZE,
        (void*) NULL,
        AMPERES_TASK_PRIO,
        Amperes_Task_Stack,
        &Amperes_Task_Buffer
    );

   // Delete Init Task
    vTaskDelete(NULL);
}