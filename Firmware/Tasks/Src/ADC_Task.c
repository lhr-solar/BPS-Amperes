#include "Tasks.h"

void ADC_Task(void *pvParameters) {
    AmperesMsg_t message;

    while (1) {
        // Block (sleep) until data arrives in queue
        Amperes_StartADC();
        xQueueReceive(adc_queue, &(message.adc_voltage), portMAX_DELAY);

        // Amperes_GetReading(&message);

        // Convert data to current measurent
        message.current_data = Amperes_ADCToCurrent(message.adc_voltage);
        
        // Send data to CAN task via queue
        xQueueSend(can_queue, &message, 0);

        // TODO: error handling for queue send
        
        // Debug
        // #ifdef DEBUG
            HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN);
        // #endif
        // vTaskDelay(pdMS_TO_TICKS(100));
    }
}
