#include "Tasks.h"

void Amperes_Task(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    AmperesMsg_t message = {0};
    uint16_t adc_sum = 0;
    int64_t current_sum = 0;
    uint8_t counter = 0;
    uint8_t conv_count = 0;

    Init_UART_Printf();

    // Temporarily using CAN RX pin for 
    // profiling loop time w logic analyzer
    GPIO_InitTypeDef debug_gpio = {
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Pin = GPIO_PIN_8 | GPIO_PIN_9
    };
    __HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_GPIO_Init(GPIOB, &debug_gpio);

    // Loop seems to take around 24 us
    while (1) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_8);  // toggle CAN RX for measuring loop time
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_8);  // toggle CAN RX for measuring loop time

        /* =================== ADC =================== */
        // Start ADC reading. Clear queue because xQueueReceive with
        // portMAX_DELAY blocks until queue goes from empty -> has data.
        if (Amperes_StartADC(true) != AMPERES_OK) { 
            // error_handler();  // TODO: handle errors
        };

        // Block until we receive data in queue
        if (Amperes_GetReading(&message, portMAX_DELAY) == AMPERES_OK) {
            conv_count++;
            adc_sum += message.adc_voltage;         // 10 sums of unsigned 12b values can fit into uint16_t
            current_sum += message.current_data;    // can probably fit yea
            HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN);
        }

        /* =================== CAN =================== */
        // Send CAN messages at 100 Hz
        if (counter >= 10 && conv_count > 0) {
            // Average data
            message.adc_voltage = (adc_sum / conv_count);
            message.current_data = (current_sum / conv_count);

            // Send data over CAN
            if (Amperes_SendCAN(&message) != AMPERES_OK) {
                // error_handler();
            }

            printf("adc = %d \r\n", message.adc_voltage);
            printf("current = %ld \r\n", message.current_data);
            printf("count = %d \r\n", conv_count);
            
            // Reset variables
            adc_sum = current_sum = 0;
            counter = conv_count = 0;

            HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_DISCHARGE_PIN);
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_9);  // toggle CAN TX for debug
        }

        /* =================== Delay =================== */
        // Using vTaskDelayUntil allows us to set a constant time period.
        // xLastWakeTime is updated within vTaskDelayUntil.
        counter++;
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_8);  // toggle CAN RX for measuring loop time
        vTaskDelayUntil(&xLastWakeTime, AMPERES_TASK_PERIOD); 
    }
}
