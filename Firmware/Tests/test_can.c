#include "stm32xx_hal.h"
#include "Amperes.h"

// CAN
#define RX_DATA_32b ((rx_data[3] << 24) | (rx_data[2] << 16) | (rx_data[1] << 8) | rx_data[0])
AmperesMsg_t message1 = {
    .adc_voltage = 25,
    .current_data = -50
};
AmperesMsg_t message2 = {
    .adc_voltage = 62,
    .current_data = 60
};

// Task
StaticTask_t xTaskBuffer;
StackType_t xStack[ 200 ];

void Task_SendCAN() {
    // CAN receive
    CAN_RxHeaderTypeDef rx_header = {0};
    uint8_t rx_data[8] = {0};
    can_status_t status;
    int32_t result;

    while (1) {
        // Send CAN data
        if (Amperes_SendCAN(&message1) != AMPERES_OK) Error_Handler();
        if (Amperes_SendCAN(&message2) != AMPERES_OK) Error_Handler();

        // Receive first payload
        status = can_recv(hcan1, 0x1, &rx_header, rx_data, portMAX_DELAY);
        result = RX_DATA_32b;
        // if (status != CAN_RECV && result != NUM1) Error_Handler();
        if (status != CAN_RECV) Error_Handler();


        // Receive second payload
        status = can_recv(hcan1, 0x1, &rx_header, rx_data, portMAX_DELAY);
        result = RX_DATA_32b;
        // if (status != CAN_RECV && result != NUM2) Error_Handler();
        if (status != CAN_RECV) Error_Handler();

        if (result) result += 0;
        
        // Blinky for verification that we are still in loop
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_HB_PIN);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main() {
    HAL_Init();
    SystemClock_Config();

    // Init LED
    GPIO_InitTypeDef led_config = {
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Pin = GPIO_PIN_5
    };
    __HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_GPIO_Init(GPIOA, &led_config);

    // LED off
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 1);

    // Make sure CAN is set to loopback
    if(Amperes_Init() == AMPERES_INIT_FAIL) Error_Handler();

    xTaskCreateStatic(Task_SendCAN,
                    "CAN Test",
                    configMINIMAL_STACK_SIZE,
                    (void*) 1,
                    tskIDLE_PRIORITY+5,
                    xStack,
                    &xTaskBuffer);

    vTaskStartScheduler();

    return 0;
}