#include "stm32xx_hal.h"
#include "Amperes.h"



int main(){
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    
    // GPIO_InitTypeDef led_config = {
    //     .Mode = GPIO_MODE_OUTPUT_PP,
    //     .Pull = GPIO_NOPULL,
    //     .Pin = AMPERES_HB_PIN
    // };
    
    // __HAL_RCC_GPIOA_CLK_ENABLE();
    // HAL_GPIO_Init(AMPERES_GPIO_PORT, &led_config); // initialize GPIOA with led_config

    while(1){
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_HB_PIN);
        HAL_Delay(500);
    }

    return 0;
}