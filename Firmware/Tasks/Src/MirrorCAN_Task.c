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
        switch (message.header.StdId) {
            case (CAN_ID_BPS_PACK_CURRENT): {
                uint8_t faults = message.data[0];
                int32_t current_mA = AMPERES_UNPACK_CURRENT_mA(message.data);
                uint8_t frame_id = message.data[4];
                printf("\r\nCurrent msg:");
                printf("\tfault: 0x%.2X | mA: %5li | frame ID: %d \r\n", faults, current_mA, frame_id);
            } break;

            case (CAN_ID_BPS_PACK_CURRENT_RAWV): {
                uint16_t raw_mV = AMPERES_UNPACK_RAW_mV(message.data);
                uint8_t frame_id = message.data[2];
                printf("\r\nRawV msg:");
                printf("\traw mV: %ld | frame ID: %d \r\n", raw_mV, frame_id);
            } break;

            default: {
                printf("\r\nUnknown ID %ld", message.header.StdId);
            } break;
        }

        portYIELD();
    }
}
