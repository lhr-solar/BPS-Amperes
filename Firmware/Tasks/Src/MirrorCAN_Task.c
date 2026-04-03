#include "Tasks.h"

void can_tx_callback_hook(CAN_HandleTypeDef* hcan, const can_tx_payload_t* payload) {
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_HB_PIN);
    xQueueSendFromISR(can_tx_queue, payload, &higherPriorityTaskWoken);
    // Don't yield from ISR
}

void MirrorCAN_Task(void *pvParameters) {
    while (1) {
        // Block on can_tx_queue
        can_tx_payload_t message;
        xQueueReceive(can_tx_queue, &message, portMAX_DELAY);

        // Print TX message
        printf("\r\n\r\nTX: ");
        for (uint8_t i=0; i < message.header.DLC; i++) {
            printf("%.2X ", message.data[i]);
        }
        
        // Print unpacked message
        int32_t current = CURRENT_CONV(message.data);
        uint16_t adc = ADC_CONV(message.data);
        printf("\r\nValue:\tmA %5li | adc %04d \r\n", current, adc);

        portYIELD();
    }
}
