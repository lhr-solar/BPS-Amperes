#include "Tasks.h"

StaticTask_t    Amperes_Task_Buffer;
StackType_t     Amperes_Task_Stack[AMPERES_TASK_STACK_SIZE];

StaticTask_t    ADC_Task_Buffer;
StackType_t     ADC_Task_Stack[ADC_TASK_STACK_SIZE];

StaticTask_t    CAN_Task_Buffer;
StackType_t     CAN_Task_Stack[CAN_TASK_STACK_SIZE];

StaticTask_t    Task_Init_Buffer;
StackType_t     Task_Init_Stack_Array[TASK_INIT_STACK_SIZE];

void Task_Init() {
    // Init Amperes GPIO, ADC, and CAN
    if (Amperes_Init(false) == AMPERES_INIT_FAIL) error_handler();
    
    // Init ADC task
    xTaskCreateStatic(
        ADC_Task,
        "ADC Task",
        ADC_TASK_STACK_SIZE,
        (void*) 1,
        ADC_TASK_PRIO,
        ADC_Task_Stack,
        &ADC_Task_Buffer
    );

    // Init CAN task
    xTaskCreateStatic(
        CAN_Task,
        "CAN Task",
        CAN_TASK_STACK_SIZE,
        (void*) 1,
        CAN_TASK_PRIO,
        CAN_Task_Stack,
        &CAN_Task_Buffer
    );

   // Delete Init Task
    vTaskDelete(NULL);
}