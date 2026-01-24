#include "Tasks.h"

void CAN_Task(void *pvParameters) {
    AmperesMsg_t message;

    while (1) {
        // Sleep until data arrives in queue
        xQueueReceive(can_queue, &message, portMAX_DELAY);

        // Send CAN message
        Amperes_SendCAN(&message);
    }
}
