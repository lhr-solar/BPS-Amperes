#include "Tasks.h"

void Amperes_Task(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    AmperesMsg_t message = {0};
    AmperesMsg_t sum = {0};
    uint8_t counter = 0;
    uint8_t conv_count = 0;

    // Loop takes ~30 us to execute
    while (1) {
        /* =================== ADC =================== */
        // Start ADC reading. Clear queue so it acts as a mailbox
        // and we can block on it being empty.

        if (Amperes_StartADC(true) != AMPERES_OK) { 
            // TODO: handle errors
        };

        // Block until we receive data in queue
        if (Amperes_GetReading(&message, AMPERES_TASK_PERIOD) == AMPERES_OK) {
            conv_count++;
            sum.adc_voltage += message.adc_voltage;
            sum.current_data += message.current_data;
        }

        /* =================== CAN =================== */
        // Send CAN messages at 100 Hz

        if (counter >= 10) {
            if (conv_count > 0) {
                // Average data and send over CAN
                message.adc_voltage = (sum.adc_voltage / conv_count);
                message.current_data = (sum.current_data / conv_count);

                // Send data over CAN
                if (Amperes_SendCAN(&message, AMPERES_TASK_PERIOD) != AMPERES_OK) {
                    // TODO: handle errors
                }

                printf("adc %d|i %li \r\n", message.adc_voltage, message.current_data);
                
                // Reset variables
                sum.adc_voltage = sum.current_data = 0;
                counter = conv_count = 0;
            } else {
                // Handle error: no adc conversions
            }
        }

        /* =================== Update =================== */
        // Using vTaskDelayUntil allows us to set a constant time period.
        // xLastWakeTime is updated within vTaskDelayUntil.
        
        Amperes_UpdateLEDs(message.current_data);
        counter++;
        vTaskDelayUntil(&xLastWakeTime, AMPERES_TASK_PERIOD); 
    }
}
