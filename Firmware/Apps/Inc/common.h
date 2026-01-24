#ifndef COMMON_H
#define COMMON_H
/**
 * Common Functionality
 */

#include "stm32xx_hal.h"

/**
 * @brief Shared error handler
 */
void error_handler();

/**
 * @brief Shared success handler
 */
void success_handler();

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

#endif