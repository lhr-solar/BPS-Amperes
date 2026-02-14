#include "Tasks.h"

void ADC_Task(void *pvParameters) {
    AmperesMsg_t message;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        // Reset queue to prevent race condition (data already in queue and task does not wake up)
        xQueueReset(adc_queue);

        // // Start ADC reading
        if (Amperes_StartADC() != AMPERES_OK) {
            error_handler();
        };

        // Block until we receive data in queue
        if (Amperes_GetReading(&message, portMAX_DELAY) == AMPERES_OK) {
            // Convert data to current measurent
            message.current_data = Amperes_ADCToCurrent(message.adc_voltage);

            // Send current data to CAN task via queue
            if(xQueueSend(can_queue, &message, 0) != pdTRUE) {
                // TODO: error handling for queue send
            }
        }
        
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
}
