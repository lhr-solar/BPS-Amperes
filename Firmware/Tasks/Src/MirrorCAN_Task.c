#include "Tasks.h"

// Reconstruct Values
#define CURRENT_CONV(x)    ( (int32_t) (x[5] << 24) | (int32_t) (x[4] << 16) | (int32_t) (x[3] << 8) | (int32_t) x[2] )
#define ADC_CONV(x)        ( (uint16_t)((x[1] << 8) | (uint16_t) x[0]) )

void can_tx_callback_hook(CAN_HandleTypeDef* hcan, const can_tx_payload_t* payload) {
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_HB_PIN);
    xQueueSendFromISR(can_tx_queue, payload, &higherPriorityTaskWoken);
    // portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

void can_rx_callback_hook(CAN_HandleTypeDef* hcan, const can_rx_payload_t* payload) {
    // HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN);
}

void MirrorCAN_Task(void *pvParameters) {
    while (1) {
        // Block on can_tx_queue
        can_tx_payload_t message;
        xQueueReceive(can_tx_queue, &message, portMAX_DELAY);

        // Print TX message
        printf("TX: ");
        for (uint32_t i=0; i < AMPERES_MSG_DLC; i++) {
            printf("%02x ", message.data[i]);
        }
        printf("\r\n");

        // Print unpacked message
        int32_t current = CURRENT_CONV(message.data);
        uint16_t adc = ADC_CONV(message.data);
        printf("Value:\tadc %04d | mA %5li \r\n\r\n", adc, current);
    }
}
