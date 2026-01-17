#include "Tasks.h"

StaticTask_t ADC_task_buffer;
StackType_t  ADC_task_stack[configMINIMAL_STACK_SIZE];

void Task_Init() {
    // Init Amperes: ADC and CAN
    if (Amperes_Init() == AMPERES_INIT_FAIL) error_handler();

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

   // Delete Init Task
    vTaskDelete(NULL);
}