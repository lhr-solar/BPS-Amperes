#include "Tasks.h"


/** ================================================================
 * CAN Mirror Test
 * ================================================================ */

StaticTask_t xTaskBuffer;
StackType_t xStack[200];

void Queue_Send() {
    can_tx_payload_t payload = {
        .header = {
            .StdId = AMPERES_MSG_ID,
            .RTR = CAN_RTR_DATA,
            .IDE = CAN_ID_STD,
            .DLC = AMPERES_MSG_DLC,
            .TransmitGlobalTime = DISABLE
        },
        .data = {0, 1, 2, 3, 4, 5}
    };
    
    while (1) {
        xQueueSend(can_tx_queue, &payload, 0);
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_DISCHARGE_PIN);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

void CAN_Send() {
    AmperesMsg_t message = {
        .adc_voltage = 20,
        .current_data = -1000
    };
    
    while (1) {
        if (Amperes_SendCAN(&message, 0) != AMPERES_OK) {
            printf("bruh\r\n");
        }
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_DISCHARGE_PIN);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

int main() {
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    if (Amperes_CAN_Init() != AMPERES_OK) Error_Handler();
    if (Amperes_CAN_Start() != AMPERES_OK) Error_Handler();
    UART_Printf_Init();

    xTaskCreateStatic(
        CAN_Send,
        "Task that sends stuff",
        200,
        (void*) 1,
        tskIDLE_PRIORITY+3,
        xStack,
        &xTaskBuffer
    );

    xTaskCreateStatic(
        MirrorCAN_Task,
        "Mirror CAN Task",
        MIRROR_CAN_TASK_STACK_SIZE,
        (void*) NULL,       
        MIRROR_CAN_TASK_PRIO,
        MirrorCAN_Task_Stack,
        &MirrorCAN_Task_Buffer
    );

    vTaskStartScheduler();

    return 0;
}
