#include "stm32xx_hal.h"
#include "Amperes.h"
#include "Tasks.h"

StaticTask_t xBlinkyTaskBuffer;
StackType_t xBlinkyStack[ 200 ];
// StaticTask_t xQueueTaskBuffer;
// StackType_t xQueueStack[ 200 ];

void Task_Blinky(void *pvParameters) {
    while (1) {
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_HB_PIN);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// void Test_Queue(void *pvParameters) {
//     int val = 3000;
//     while (1) {
//         xQueueSend(adc_queue, &val, 0);
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }

int main() {
    // __HAL_DBGMCU_FREEZE_TIM6();
    HAL_Init();
    SystemClock_Config();
    
    if(Amperes_Init(false) == AMPERES_INIT_FAIL) error_handler();

    xTaskCreateStatic(
        ADC_Task,
        "ADC Task",
        ADC_TASK_STACK_SIZE,
        (void*) 1,
        ADC_TASK_PRIO,
        ADC_task_stack,
        &ADC_task_buffer
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

    // xTaskCreateStatic(
    //     Test_Queue,
    //     "Queue Send",
    //     200,
    //     (void*) 1,
    //     tskIDLE_PRIORITY+4,
    //     xQueueStack,
    //     &xQueueTaskBuffer
    // );

    vTaskStartScheduler();

    return 0;
}