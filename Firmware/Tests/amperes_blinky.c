#include "stm32xx_hal.h"
#include "Amperes.h"

// Initialize clock for heartbeat LED port
void Heartbeat_Clock_Init() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
}

int main(){
    HAL_Init();

    GPIO_InitTypeDef led_config = {
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Pin = AMPERES_HB_PIN
    };
    
    Heartbeat_Clock_Init(); // enable clock for LED_PORT
    HAL_GPIO_Init(AMPERES_LED_PORT, &led_config); // initialize GPIOA with led_config

    while(1){
        HAL_GPIO_TogglePin(AMPERES_LED_PORT, AMPERES_HB_PIN);
        HAL_Delay(500);
    }

    return 0;
}