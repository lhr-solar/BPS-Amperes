#include "stm32xx_hal.h"
#include "Amperes.h"

// CAN
#define CURRENT_CONV    ((int32_t) ((rx_data[5] << 24) | (rx_data[4] << 16) | (rx_data[3] << 8) | rx_data[2]))
#define ADC_CONV        ((uint16_t) ((rx_data[1] << 8) | rx_data[0]))
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
StackType_t xStack[200];

void Task_SendCAN() {
    // CAN receive
    CAN_RxHeaderTypeDef rx_header = {0};
    uint8_t rx_data[6] = {0};
    can_status_t status;
    uint16_t adc;
    int32_t current;

    while (1) {
        // Send CAN data
        if (Amperes_SendCAN(&message1, portMAX_DELAY) != AMPERES_OK) {
            printf("no can send \r\n");
            Error_Handler();
        }
        // if (Amperes_SendCAN(&message2, portMAX_DELAY) != AMPERES_OK) Error_Handler();
        printf("SENT: adc %d | current %li \r\n", message1.adc_voltage, message1.current_data);
        vTaskDelay(pdMS_TO_TICKS(10));

        // Receive first payload
        status = can_recv(hcan1, AMPERES_CAN_STD_ID, &rx_header, rx_data, portMAX_DELAY);
        if (status != CAN_RECV) {
            if (status == CAN_EMPTY) {
                printf("can empty :(\r\n");
            } else if (status == CAN_ERR) {
                printf("can error (id) :(\r\n");
            }
            Error_Handler();
        }
        adc = ADC_CONV;
        current = CURRENT_CONV;
        printf("RECEIVED: adc %d | current %li \r\n", adc, current);
        if ((adc != message1.adc_voltage) && (current != message1.current_data)) {
            Error_Handler();
        }


        // Receive second payload
        // status = can_recv(hcan1, 0x1, &rx_header, rx_data, portMAX_DELAY);
        // adc = ADC_CONV;
        // current = CURRENT_CONV;
        // if (status == CAN_RECV) Error_Handler();
        // if ((adc != message2.adc_voltage) && (current != message2.current_data)) {
        //     Error_Handler();
        // }
        
        // Blinky for verification that we are still in loop
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_HB_PIN);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main() {
    HAL_Init();
    SystemClock_Config();
    MX_UART_Init();
    UART_Printf_Init();

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