#include "AmperesConfig.h"
#include "AmperesCAN.h"
#include "common.h"

/** ================================================================
 * CAN Test
 * - Sends Amperes CAN messages (loopback) and checks 
 *   that they were received correctly. 
 * - Debug w/ printf and a CAN adapter.
 * ================================================================ */

// Reconstruct Values
#define CURRENT_CONV    ( (int32_t) (rx_data[5] << 24) | (int32_t) (rx_data[4] << 16) | (int32_t) (rx_data[3] << 8) | (int32_t) rx_data[2] )
#define ADC_CONV        ( (uint16_t)((rx_data[1] << 8) | (uint16_t) rx_data[0]) )

StaticTask_t xTaskBuffer;
StackType_t xStack[200];

void Task_SendCAN() {
    // Receive
    CAN_RxHeaderTypeDef rx_header = {0};
    uint8_t rx_data[6] = {0};
    can_status_t status;
    uint16_t adc;
    int32_t current;

    // Transmit
    AmperesMsg_t payload = {
        .adc_voltage = 4095,
        .current_data = 80000
    };
    
    while (1) {
        // Increment data
        if (payload.adc_voltage < 15) {
            payload.adc_voltage = 4095;
        } else {
            payload.adc_voltage -= 15;
        }
        if (payload.current_data == -30000) {
            payload.current_data = 80000;
        } else {
            payload.current_data -= 1000;
        }

        // Send CAN data
        if (Amperes_SendCAN(&payload, portMAX_DELAY) != AMPERES_OK) {
            printf("no can send \r\n");
            Error_Handler();
        }
        printf("\r\nSEND \t adc %4d | current %5li \r\n", payload.adc_voltage, payload.current_data);

        // Receive payload
        status = can_recv(hcan1, AMPERES_MSG_ID, &rx_header, rx_data, portMAX_DELAY);
        if (status != CAN_RECV) {
            if (status == CAN_EMPTY) {
                printf("can empty :(\r\n");
            } else if (status == CAN_ERR) {
                printf("can error (id) :(\r\n");
            }
            Error_Handler();
        }

        // Convert back hopefully
        adc = ADC_CONV;
        current = CURRENT_CONV;
        printf("RECV \t adc %4d | current %5li \r\n", adc, current);
        if ((adc != payload.adc_voltage) || (current != payload.current_data)) {
            Error_Handler();
        }
        printf("Match! \r\n");

        // Blinky for verification that we are still in loop
        HAL_GPIO_TogglePin(AMPERES_GPIO_PORT, AMPERES_HB_PIN);

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

int main() {
    HAL_Init();
    SystemClock_Config();
    UART_Printf_Init();

    // Make sure CAN is set to loopback
    MX_GPIO_Init();
    if (Amperes_CAN_Init() != AMPERES_OK) Error_Handler();
    if (Amperes_CAN_Start() != AMPERES_OK) Error_Handler();

    xTaskCreateStatic(Task_SendCAN,
                    "CAN Test",
                    200,
                    (void*) 1,
                    tskIDLE_PRIORITY+5,
                    xStack,
                    &xTaskBuffer);

    vTaskStartScheduler();

    return 0;
}