#include "Tasks.h"

void Amperes_Task(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    AmperesMsg_t message = {0};
    uint16_t adc_sum = 0;
    int64_t current_sum = 0;
    uint8_t counter = 0;
    uint8_t conv_count = 0;

    while (1) {
        /* =================== ADC =================== */
        // Reset queue to prevent race condition 
        // xQueueReceive with portMAX_DELAY unblocks when queue goes from empty -> something
        xQueueReset(adc_queue);

        // Start ADC reading
        if (Amperes_StartADC() != AMPERES_OK) { 
            // error_handler();
        };

        // Block until we receive data in queue
        if (Amperes_GetReading(&message, portMAX_DELAY) == AMPERES_OK) {
            conv_count++;
            adc_sum += message.adc_voltage;         // ok because 10 * 12b inputs can fit in 16 bits
            current_sum += message.current_data;    // can probably fit yea
            HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN);
        }

        /* =================== CAN =================== */
        // Send CAN messages at 100 Hz
        if (counter >= 10) {
            // Reset counters
            conv_count = 0;
            counter = 0;

            // Average and convert data to current measurement
            message.adc_voltage = (adc_sum / conv_count);
            message.current_data = (current_sum / conv_count);
            
            // Send data over CAN
            if (Amperes_SendCAN(&message) != AMPERES_OK) {
                // error_handler();
            }
            HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_DISCHARGE_PIN);
        }

        /* =================== Update + delay =================== */
        // Using vTaskDelayUntil allows us to set a constant time period.
        // xLastWakeTime is updated within vTaskDelayUntil.
        counter++;
        vTaskDelayUntil(&xLastWakeTime, AMPERES_TASK_PERIOD); 
    }
}
