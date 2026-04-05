#include "AmperesConfig.h"
#include "AmperesCAN.h"
#include "common.h"

/** ================================================================
 * CAN Test
 * - Sends Amperes CAN messages (loopback) and checks 
 *   that they were received correctly. 
 * - Debug w/ printf and a CAN adapter.
 * !!! CAN mode must be set to LOOPBACK and filter must accept ID
 * ================================================================ */

StaticTask_t xTaskBuffer;
StackType_t xStack[200];

void Task_CAN_Loopback() {
    // Receive
    CAN_RxHeaderTypeDef rx_header = {0};
    uint8_t rx_data[AMPERES_MSG_DLC] = {0};
    can_status_t status;
    int32_t current_mA;
    uint16_t raw_mV;

    // Transmit
    bps_pack_current_t payload = {
        .Main_Battery_Current = 80000,
        .Main_Battery_Current_RawV = 4095        
    };
    
    while (1) {
        // Increment data
        if (payload.Main_Battery_Current == -30000) {
            payload.Main_Battery_Current = 80000;
        } else {
            payload.Main_Battery_Current -= 1000;
        }
        if (payload.Main_Battery_Current_RawV < 15) {
            payload.Main_Battery_Current_RawV = 4095;
        } else {
            payload.Main_Battery_Current_RawV -= 15;
        }

        // Send CAN data
        if (Amperes_SendCAN(&payload, portMAX_DELAY) != AMPERES_OK) {
            printf("no can send \r\n");
            Error_Handler();
        }
        printf("\r\nSEND \t current %5li | adc %4d \r\n", payload.Main_Battery_Current, payload.Main_Battery_Current_RawV);

        // Receive payload; make sure CAN filter in Amperes_CAN_Init will accept this ID
        status = can_recv(hcan1, AMPERES_MSG_ID, &rx_header, rx_data, portMAX_DELAY);
        if (status != CAN_OK) {
            if (status == CAN_EMPTY) {
                printf("can empty :(\r\n");
            } else if (status == CAN_ERR) {
                printf("can error (id) :(\r\n");
            }
            Error_Handler();
        }

        // Convert back hopefully
        current_mA = AMPERES_UNPACK_CURRENT_mA(rx_data);
        raw_mV = AMPERES_UNPACK_RAW_mV(rx_data);
        printf("RECV \t current %5li | raw_mV %4d \r\n", current_mA, raw_mV);
        if ((current_mA != payload.Main_Battery_Current) || (raw_mV != payload.Main_Battery_Current_RawV)) {
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

    // Make sure CAN is set to loopback; makefile should handle this
    MX_GPIO_Init();
    if (Amperes_CAN_Init() != AMPERES_OK) Error_Handler();
    if (Amperes_CAN_Start() != AMPERES_OK) Error_Handler();

    xTaskCreateStatic(Task_CAN_Loopback,
                    "CAN Test",
                    200,
                    (void*) 1,
                    tskIDLE_PRIORITY+5,
                    xStack,
                    &xTaskBuffer);

    vTaskStartScheduler();

    return 0;
}