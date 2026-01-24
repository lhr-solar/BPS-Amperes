#include "Tasks.h"

StaticTask_t ADC_task_buffer;
StackType_t  ADC_task_stack[configMINIMAL_STACK_SIZE];

StaticTask_t CAN_task_buffer;
StackType_t  CAN_task_stack[configMINIMAL_STACK_SIZE];

StaticTask_t Task_Init_Buffer;
StackType_t Task_Init_Stack_Array[configMINIMAL_STACK_SIZE];

void Task_Init() {
    // Init Amperes ADC and CAN
    if (Amperes_Init(true) == AMPERES_INIT_FAIL) error_handler();

    // Init ADC task
    xTaskCreateStatic(
        ADC_Task,
        "ADC Task",
        configMINIMAL_STACK_SIZE,
        (void*) 1,
        ADC_PRIO,
        ADC_task_stack,
        &ADC_task_buffer
    );

    // Init CAN task
    xTaskCreateStatic(
        CAN_Task,
        "CAN Task",
        configMINIMAL_STACK_SIZE,
        (void*) 1,
        CAN_PRIO,
        CAN_task_stack,
        &CAN_task_buffer
    );

   // Delete Init Task
    vTaskDelete(NULL);
}