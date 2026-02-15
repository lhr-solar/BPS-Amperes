#include "stm32xx_hal.h"
#include "Amperes.h"
#include "Tasks.h"

StaticTask_t xBlinkyTaskBuffer;
StackType_t xBlinkyStack[ 200 ];
StaticTask_t xQueueTaskBuffer;
StackType_t xQueueStack[ 200 ];
StaticTask_t xADCTaskBuffer;
StackType_t xADCStack[ 200 ];

void Task_Blinky(void *pvParameters) {
    while (1) {
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_HB_PIN);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void Test_Queue(void *pvParameters) {
    int val = 3000; // 3V
    while (1) {
        xQueueSend(adc_queue, &val, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void Test_ADC(void *pvParameters) {
    AmperesMsg_t message;
    while (1) {
        // Block (sleep) until data arrives in queue
        xQueueReceive(adc_queue, &(message.adc_voltage), portMAX_DELAY);

        // Convert data to current measurent
        message.current_data = Amperes_ADCToCurrent(message.adc_voltage);
        
        // Send data to CAN task via queue
        // xQueueSend(can_queue, &message, 0);
        
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN);
        vTaskDelay(message.adc_voltage); 
    }
}

int main() {
    HAL_Init();
    SystemClock_Config();
    
    if(Amperes_Init() == AMPERES_INIT_FAIL) error_handler();

    xTaskCreateStatic(
        Test_ADC,
        "ADC",
        ADC_TASK_STACK_SIZE,
        (void*) 1,
        tskIDLE_PRIORITY+3,
        xADCStack,
        &xADCTaskBuffer
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

    xTaskCreateStatic(
        Test_Queue,
        "Queue Send",
        200,
        (void*) 1,
        tskIDLE_PRIORITY+4,
        xQueueStack,
        &xQueueTaskBuffer
    );

    vTaskStartScheduler();

    return 0;
}