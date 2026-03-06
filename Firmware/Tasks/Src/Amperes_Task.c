#include "Tasks.h"

#define PRINTF_ENABLED 1

void Amperes_Task(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    AmperesMsg_t message = {0};
    AmperesMsg_t sum = {0};

    uint8_t counter = 0;
    uint8_t conv_count = 0;
    // uint8_t adc_errors = 0;
    // uint8_t can_errors = 0;

    // Loop takes ~30 us to execute
    while (1) {
        /* ============= Track loop count ============= */
        counter++;

        /* =================== ADC =================== */
        // Start ADC reading. Clear queue so it acts as a mailbox
        // and we can block on it being empty.
        if (Amperes_StartADC(true) != AMPERES_OK) { 
            // TODO: handle errors. restart ADC ?
        }

        // Block until we receive data in queue
        if (Amperes_GetReading(&message, AMPERES_TASK_PERIOD) == AMPERES_OK) {
            conv_count++;
            sum.adc_voltage += message.adc_voltage;
            sum.current_data += message.current_data;
        } else {
            // handle error
            // adc_errors++;
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
                    // can_errors++;
                }

                #ifdef PRINTF_ENABLED
                printf("\r\n adc %d|i %li| conv %d \r\n", message.adc_voltage, message.current_data, conv_count);
                #endif
                
                // Reset variables
                sum.adc_voltage = sum.current_data = 0;
                counter = conv_count = 0;
            } else {
                // Handle error: no adc conversions
                // reinit ADC?
            }
            
            // Update LED values
            if (message.current_data < 0) {
                // Negative means charging
                HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN, GPIO_PIN_SET);
                HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_DISCHARGE_PIN, GPIO_PIN_RESET);
            } else {
                // Positive means discharging
                HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_DISCHARGE_PIN, GPIO_PIN_SET);
                HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN, GPIO_PIN_RESET);
            }
        }

        /* =================== Handle Errors =================== */
        // if (adc_errors > 10) {
        //     // reinit adc ?
        //     adc_errors = 0;
        // }
        // if (can_errors > 10) {
        //     Error_Handler();
        // }

        /* =================== Period Delay =================== */
        // Using vTaskDelayUntil allows us to set a constant time period.
        vTaskDelayUntil(&xLastWakeTime, AMPERES_TASK_PERIOD); 
    }
}
