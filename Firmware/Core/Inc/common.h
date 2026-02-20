#pragma once

/**
 * Common Functionality
 */

#include "stm32xx_hal.h"
#include "printf.h"

/**
 * @brief Shared error handler
 */
void Error_Handler();


/**
 * @brief Shared success handler
 */
void Success_Handler();

/**
  * @brief System Clock Configuration: 80 MHz
  * @retval None
  */
void SystemClock_Config(void);

/**
  * @brief Initializes Amperes GPIO - 
  *        Heartbeat, Fault, Charge, Discharge LEDS; Boot GPIO.
  * @retval None
  */
void MX_GPIO_Init(void);

void MX_UART_Init();
void Init_UART_Printf();