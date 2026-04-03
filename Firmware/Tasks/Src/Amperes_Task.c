#include "Tasks.h"

#ifndef PRINTF_ENABLED
    #define PRINTF_ENABLED 0
#endif

#define CAN_SEND_PERIOD_MS 100
#define CAN_SEND_PERIOD_COUNT (CAN_SEND_PERIOD_MS / AMPERES_TASK_PERIOD_MS)

void Amperes_Task(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    bps_pack_current_t message = {0};
    int32_t current_sum = 0;
    uint32_t adc_sum = 0;

    uint8_t counter = 0;    // loop counter
    uint8_t conv_count = 0; // adc conversion count
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
            #if PRINTF_ENABLED
            printf("ADC Start Error\r\n");
            #endif
        }

        // Block until we receive data in queue
        if (Amperes_GetReading(&message, pdMS_TO_TICKS(AMPERES_TASK_PERIOD_MS)) == AMPERES_OK) {
            conv_count++;
            current_sum += message.Main_Battery_Current;
            adc_sum += message.Main_Battery_Current_RawV;
        } else {
            #if PRINTF_ENABLED
            printf("ADC Reading Error\r\n");
            #endif
        }

        /* =================== CAN =================== */
        // Send CAN messages at 10 Hz
        if (counter >= CAN_SEND_PERIOD_COUNT) {
            if (conv_count > 0) {
                // Average data and send over CAN
                message.Main_Battery_Current = (current_sum / conv_count);
                message.Main_Battery_Current_RawV = (uint16_t) (adc_sum / conv_count);

                // Send data over CAN
                if (Amperes_SendCAN(&message, pdMS_TO_TICKS(AMPERES_TASK_PERIOD_MS)) != AMPERES_OK) {
                    #if PRINTF_ENABLED
                    printf("CAN Send Error\r\n");
                    #endif
                }

                #if PRINTF_ENABLED
                printf("\r\n i %li| adc %d | conv %d \r\n", message.Main_Battery_Current, message.Main_Battery_Current_RawV, conv_count);
                #endif
                
                // Reset variables
                current_sum = adc_sum = 0;
                counter = conv_count = 0;
            } else {
                // Error: no adc conversions
                #if PRINTF_ENABLED
                printf("No ADC Conversions\r\n");
                #endif
            }
            
            // Update LED values
            if (message.Main_Battery_Current < 0) {
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
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(AMPERES_TASK_PERIOD_MS)); 
    }
}
