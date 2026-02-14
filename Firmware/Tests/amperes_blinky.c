#include "stm32xx_hal.h"
#include "Amperes.h"

int main(){
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    while(1){
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_HB_PIN);
        HAL_Delay(125);
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_FAULT_PIN);
        HAL_Delay(125);
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN);
        HAL_Delay(125);
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_DISCHARGE_PIN);
        HAL_Delay(125);
    }

    return 0;
}