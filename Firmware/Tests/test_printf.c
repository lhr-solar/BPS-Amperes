#include "common.h"

/** ================================================================
 * Printf Test
 * - Prints stuff, that's about it
 * ================================================================ */

StaticTask_t txTaskBuffer;
StackType_t txTaskStack[configMINIMAL_STACK_SIZE*4];


// HAL_UART_MspGPIOInit is defined in msp_inits.c

void TxTask(void *argument) {
    UART_Printf_Init();
    while(1) {
        printf("Hello World! %s %d\n\r", "Test String", 5);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();

    xTaskCreateStatic(TxTask, 
                     "TX",
                     configMINIMAL_STACK_SIZE*4,
                     NULL,
                     tskIDLE_PRIORITY + 2,
                     txTaskStack,
                     &txTaskBuffer);

    vTaskStartScheduler();

    while (1) {
    }
}
