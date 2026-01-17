#include "Tasks.h"

void ADC_Task(void *pvParameters) {
    while (1) {
        // Get Amperes current reading
        int32_t current_value;
        if(Amperes_GetReading(&current_value) != AMPERES_OK) {
            // do something or idk
        }

        // Send data over CAN


        // Debug
        #ifdef DEBUG
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        #endif

        // TODO: don't poll, block and wake up task instead
        vTaskDelay(ADC_DELAY);
    }
}
