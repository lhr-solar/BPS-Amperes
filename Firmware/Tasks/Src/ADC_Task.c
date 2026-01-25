#include "Tasks.h"

void ADC_Task(void *pvParameters) {
    uint16_t adc_value;
    int32_t current_value;
    AmperesMsg_t message;

    while (1) {
        // Block (sleep) until data arrives in queue
        xQueueReceive(adc_queue, &adc_value, portMAX_DELAY);

        // Convert data to current measurent
        current_value = Amperes_ADCToCurrent(adc_value);
        
        // Send data to CAN task via queue
        message.adc_voltage = adc_value;
        message.current_data = current_value;
        xQueueSend(can_queue, &message, 0);

        // TODO: error handling for queue send
        
        // Debug
        #ifdef DEBUG
            HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN);
        #endif
    }
}
