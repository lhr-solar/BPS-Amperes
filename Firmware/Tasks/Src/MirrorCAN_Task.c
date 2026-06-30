#include "Tasks.h"

void can_tx_callback_hook(CAN_HandleTypeDef* hcan, const can_tx_payload_t* payload) {
    static uint8_t toggle_count = 0; // toggle every other send (two sends in a row)
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    if (toggle_count) {
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_HB_PIN);
    }
    xQueueSendFromISR(can_tx_queue, payload, &higherPriorityTaskWoken);
    toggle_count = (toggle_count + 1) & 0x1;
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
                bps_pack_current_t result = {0};
                Amperes_Unpack_Current_mA(message.data, &result);
                printf("\r\nCurrent msg:");
                printf("\t[fault: 0x%.2X | mA: %5li | frame ID: %d]\r\n", 
                        result.BPS_Amperes_Fault, 
                        result.Main_Battery_Current, 
                        result.FrameID_Amperes);
            } break;

            case (CAN_ID_BPS_PACK_CURRENT_ADC): {
                bps_pack_current_adc_t result = {0};
                Amperes_Unpack_ADC(message.data, &result);
                printf("\r\nADC msg:");
                printf("\t[adc counts: %u | frame ID: %d]\r\n", 
                        result.Main_Battery_Current_ADC,
                        result.FrameID_Amperes);
            } break;

            default: {
                printf("\r\nUnknown ID %ld", message.header.StdId);
            } break;
        }

        portYIELD();
    }
}
