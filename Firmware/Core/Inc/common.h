#pragma once

/** ================================================================
 *  Common Functionality: Handlers, User-Callable Configs/Inits
 * ================================================================ */

#include "stm32xx_hal.h"
#include "AmperesConfig.h"
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
  *   LEDS (Heartbeat, Fault, Charge, Discharge) and Boot GPIO.
  * @retval None
  */
void MX_GPIO_Init(void);

/**
 * @brief Initializes UART for printf serial debugging.
 * @retval True if init successful.
 */
bool UART_Printf_Init();