#include "Tasks.h"


#define CAN_SEND_PERIOD_MS 25
#define CAN_SEND_PERIOD_COUNT (CAN_SEND_PERIOD_MS / AMPERES_TASK_PERIOD_MS)

#define MAX_FRAME_ID 256

// Amperes Overcurrent Setpoints: 70A discharge, 40A charge
#define OVERCURRENT_DISCHARGE_THRESHOLD_mA  (68000)
#define OVERCURRENT_CHARGE_THRESHOLD_mA     (-38000)

// Out-of-bounds Setpoints: 90A discharge, -60A charge
#define OUT_OF_BOUNDS_DISCHARGE_THRESHOLD_mA   (90000)
#define OUT_OF_BOUNDS_CHARGE_THRESHOLD_mA      (-60000)

// Error messages when ADC is broken (no converions at all)
#define CURRENT_ERROR_MSG   69420
#define ADC_ERROR_MSG     6769

// Thresholds for sending watchdog fault 
#define ADC_WATCHDOG_COUNT      500     // 500 ms of missing adc readings
#define CAN_SEND_WATCHDOG_COUNT 10      // 1s of missing can send

void Amperes_Task(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    int32_t current_sum = 0;
    uint32_t adc_sum = 0;

    uint16_t counter = 0;           // loop counter
    uint16_t conv_count = 0;        // adc conversion count
    uint16_t adc_read_error = 0;    // adc read misses
    uint16_t can_send_error = 0;    // can send misses

    // frame ID used to sync amperes pack current and rawMV messages
    uint8_t frame_id = 0;

    // Loop takes ~30 us to execute
    while (1) {
        /* ============= Track loop count ============= */
        counter++;

        /* =================== ADC =================== */
        // Start ADC reading. Clear queue so it acts as a mailbox and we can block on it being empty.
        if (Amperes_StartADC(true) != AMPERES_OK) {
            #if PRINTF_ENABLED
            printf("ADC Start Error\r\n");
            #endif
        } else {
            uint16_t adc_reading = 0;
            // Block until we receive data in queue
            if (Amperes_GetReading(&adc_reading, pdMS_TO_TICKS(AMPERES_TASK_PERIOD_MS)) == AMPERES_OK) {
                // Add data to sum (accumulate raw 12-bit ADC counts; the DBC field is now ADC counts, not mV)
                adc_sum += adc_reading;
                current_sum += Amperes_ADCToCurrent(adc_reading);
                conv_count++;
                adc_read_error = 0; // reset read error count
            } else {
                if (adc_read_error < UINT16_MAX) adc_read_error++;
                #if PRINTF_ENABLED
                printf("ADC Reading Error\r\n");
                #endif
            }
        }

        /* =================== CAN =================== */
        // Send CAN messages at 10 Hz
        if (counter >= CAN_SEND_PERIOD_COUNT) {
            if (conv_count > 0) {
                bps_pack_current_t message_packCurr = {0};
                bps_pack_current_adc_t message_adc = {0};

                /* ============= Current Msg ============= */
                message_packCurr.Main_Battery_Current = (current_sum / conv_count);
                message_packCurr.FrameID_Amperes = frame_id;

                // Handle faults
                if ((adc_read_error > ADC_WATCHDOG_COUNT) || (can_send_error > CAN_SEND_WATCHDOG_COUNT)) {
                    // Watchdog fault
                    message_packCurr.BPS_Amperes_Fault = BPS_PACK_CURRENT_BPS_AMPERES_FAULT_MESSAGE_WATCHDOG;
                } else if ((message_packCurr.Main_Battery_Current < OUT_OF_BOUNDS_CHARGE_THRESHOLD_mA) ||
                    (message_packCurr.Main_Battery_Current > OUT_OF_BOUNDS_DISCHARGE_THRESHOLD_mA) ) {
                    // Out of bounds fault
                    message_packCurr.BPS_Amperes_Fault = BPS_PACK_CURRENT_BPS_AMPERES_FAULT_OUT_OF_BOUNDS;
                } else if (message_packCurr.Main_Battery_Current < OVERCURRENT_CHARGE_THRESHOLD_mA) {
                    // Overcurrent faults
                    
                    message_packCurr.BPS_Amperes_Fault = BPS_PACK_CURRENT_BPS_AMPERES_FAULT_OVER_CURRENT_CHARGE_;
                } else if (message_packCurr.Main_Battery_Current > OVERCURRENT_DISCHARGE_THRESHOLD_mA) {
                    message_packCurr.BPS_Amperes_Fault = BPS_PACK_CURRENT_BPS_AMPERES_FAULT_OVER_CURRENT_DISCHARGE_;
                } else {
                    // Amperes OK
                    message_packCurr.BPS_Amperes_Fault = BPS_PACK_CURRENT_BPS_AMPERES_FAULT_OK;
                }

                if (message_packCurr.BPS_Amperes_Fault == BPS_PACK_CURRENT_BPS_AMPERES_FAULT_OK) {
                    HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_FAULT_PIN, GPIO_PIN_RESET);
                } else {
                    HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_FAULT_PIN, GPIO_PIN_SET);
                }


                /* ============= ADC Msg ============= */
                message_adc.Main_Battery_Current_ADC = (adc_sum / conv_count);
                message_adc.FrameID_Amperes = frame_id;
            
                // Send data over CAN
                uint8_t can_send_failed = 0;
                if (Amperes_SendPackCurrentCAN(&message_packCurr, pdMS_TO_TICKS(AMPERES_TASK_PERIOD_MS)) != AMPERES_OK) {
                    can_send_failed = 1;
                    #if PRINTF_ENABLED
                    printf("Pack current CAN Send Error\r\n");
                    #endif
                }
                if (Amperes_SendPackCurrentADC(&message_adc, pdMS_TO_TICKS(AMPERES_TASK_PERIOD_MS)) != AMPERES_OK) {
                    can_send_failed = 1;
                    #if PRINTF_ENABLED
                    printf("Pack mV CAN Send Error\r\n");
                    #endif
                }

                if (can_send_failed) {
                    if (can_send_error < UINT16_MAX) can_send_error++;
                } else can_send_error = 0;

                #if PRINTF_ENABLED
                printf("\r\nAmperes Task:\t[i %li| adc %u | conv %d]\r\n", 
                        message_packCurr.Main_Battery_Current, 
                        message_adc.Main_Battery_Current_ADC, 
                        conv_count);
                #endif

                // Update LED values
                if (message_packCurr.Main_Battery_Current < 0) {
                    // Negative means charging
                    HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_DISCHARGE_PIN, GPIO_PIN_RESET);
                } else {
                    // Positive means discharging
                    HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_DISCHARGE_PIN, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_CHARGE_PIN, GPIO_PIN_RESET);
                }

            } else {
                /* ============= ERROR: No ADC conversions ============= */
                bps_pack_current_t message_packCurr = {0};
                bps_pack_current_adc_t message_adc = {0};

                // Watchdog fault
                message_packCurr.BPS_Amperes_Fault = (1 << BPS_PACK_CURRENT_BPS_AMPERES_FAULT_MESSAGE_WATCHDOG);

                /* ============= Current Msg ============= */
                message_packCurr.Main_Battery_Current = CURRENT_ERROR_MSG;
                message_packCurr.FrameID_Amperes = frame_id;

                /* ============= ADC Msg ============= */
                message_adc.Main_Battery_Current_ADC = ADC_ERROR_MSG;
                message_adc.FrameID_Amperes = frame_id;

                // Send data over CAN
                uint8_t can_send_failed = 0;
                if (Amperes_SendPackCurrentCAN(&message_packCurr, pdMS_TO_TICKS(AMPERES_TASK_PERIOD_MS)) != AMPERES_OK) {
                    can_send_failed = 1;
                    #if PRINTF_ENABLED
                    printf("Pack current CAN Send Error\r\n");
                    #endif
                }
                if (Amperes_SendPackCurrentADC(&message_adc, pdMS_TO_TICKS(AMPERES_TASK_PERIOD_MS)) != AMPERES_OK) {
                    can_send_failed = 1;
                    #if PRINTF_ENABLED
                    printf("Pack mV CAN Send Error\r\n");
                    #endif
                }
                if (can_send_failed) {
                    if (can_send_error < UINT16_MAX) can_send_error++;
                } else can_send_error = 0;

                #if PRINTF_ENABLED
                printf("No ADC Conversions\r\n");
                #endif
                HAL_GPIO_WritePin(AMPERES_GPIO_PORT, AMPERES_FAULT_PIN, GPIO_PIN_SET);
            }
            // Update variables
            frame_id = (frame_id + 1) % MAX_FRAME_ID;
            current_sum = adc_sum = 0;
            counter = conv_count = 0;
        }

        /* =================== Period Delay =================== */
        // Using vTaskDelayUntil allows us to set a constant time period.
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(AMPERES_TASK_PERIOD_MS)); 
    }
}
