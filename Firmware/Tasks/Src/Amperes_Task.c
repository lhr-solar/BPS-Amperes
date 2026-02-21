#include "Tasks.h"

void Amperes_Task(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    AmperesMsg_t message = {0};
    AmperesMsg_t sum = {0};
    uint8_t counter = 0;
    uint8_t conv_count = 0;

    // Temporarily using CAN RX pin for 
    // profiling loop time w logic analyzer
    // GPIO_InitTypeDef debug_gpio = {
    //     .Mode = GPIO_MODE_OUTPUT_PP,
    //     .Pull = GPIO_NOPULL,
    //     .Pin = GPIO_PIN_8 | GPIO_PIN_9
    // };
    // __HAL_RCC_GPIOB_CLK_ENABLE();
    // HAL_GPIO_Init(GPIOB, &debug_gpio);

    // Loop seems to take around 24 us
    while (1) {
        // HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_8);  // toggle CAN RX for measuring loop time
        // HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_8);  // toggle CAN RX for measuring loop time

        /* =================== ADC =================== */
        // Start ADC reading. Clear queue so it acts as a mailbox
        // and we can block on it being empty.
        if (Amperes_StartADC(true) != AMPERES_OK) { 
            // Error_Handler();  // TODO: handle errors
        };

        // Block until we receive data in queue
        if (Amperes_GetReading(&message, AMPERES_TASK_PERIOD) == AMPERES_OK) {
            conv_count++;
            sum.adc_voltage += message.adc_voltage;
            sum.current_data += message.current_data;
            HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN);
        }

        /* =================== CAN =================== */
        // Send CAN messages at 100 Hz
        if (counter >= 10) {
            if (conv_count > 0) {
                // Average data
                message.adc_voltage = (sum.adc_voltage / conv_count);
                message.current_data = (sum.current_data / conv_count);

                // Send data over CAN
                if (Amperes_SendCAN(&message) != AMPERES_OK) {
                    // Error_Handler();
                }

                printf("adc = %d | i = %li \r\n", message.adc_voltage, message.current_data);
                
                // Reset variables
                sum.adc_voltage = sum.current_data = 0;
                counter = conv_count = 0;

                HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_DISCHARGE_PIN);
            } else {
                // handle error: no adc conversions
            }
            // HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_9);  // toggle CAN TX for debug
        }

        // HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_8);  // toggle CAN RX for measuring loop time

        /* =================== Delay =================== */
        // Using vTaskDelayUntil allows us to set a constant time period.
        // xLastWakeTime is updated within vTaskDelayUntil.
        counter++;
        vTaskDelayUntil(&xLastWakeTime, AMPERES_TASK_PERIOD); 
    }
}
