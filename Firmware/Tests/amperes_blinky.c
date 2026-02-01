#include "stm32xx_hal.h"
#include "Amperes.h"

int main(){
    HAL_Init();
    SystemClock_Config();

    while(1){
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_HB_PIN);
        HAL_Delay(500);
    }

    return 0;
}