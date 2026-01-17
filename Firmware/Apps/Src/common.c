#include "common.h"
#include "Amperes.h"

void error_handler() {
    // Turn on fault LED
    HAL_GPIO_WritePin(AMPERES_LED_PORT, AMPERES_FAULT_PIN, GPIO_PIN_SET);
    while (1) {}
}

void success_handler() {
    // Blink HB LED
    while(1){
        HAL_GPIO_TogglePin(AMPERES_LED_PORT, AMPERES_HB_PIN);
        HAL_Delay(500);
    }
}
