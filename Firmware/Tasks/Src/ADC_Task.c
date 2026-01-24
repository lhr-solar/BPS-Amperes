#include "Tasks.h"

void ADC_Task(void *pvParameters) {
    int32_t adc_value;
    int32_t current_value;
    AmperesMsg_t message;

    while (1) {
        // Sleep until data arrives in queue
        xQueueReceive(adc_queue, &adc_value, portMAX_DELAY);

        // Convert data to current measurent
        current_value = AMPERES_ADC_TO_CURRENT(adc_value);
        
        // Send data to CAN task via queue
        message.adc_voltage = adc_value;
        message.current_data = current_value;
        xQueueSend(can_queue, &message, 0);

        // Debug
        #ifdef DEBUG
            HAL_GPIO_TogglePin(AMPERES_LED_PORT, AMPERES_CHARGE_PIN);
        #endif
    }
}
