#include "Tasks.h"

void CAN_Task(void *pvParameters) {
    AmperesMsg_t message;

    while (1) {
        // Block (sleep) until data arrives in queue
        xQueueReceive(can_queue, &message, portMAX_DELAY);

        // Send CAN message
        if (Amperes_SendCAN(&message) != AMPERES_OK) {
            // Handle error
        }

        // Debug
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_DISCHARGE_PIN);
    }
}
