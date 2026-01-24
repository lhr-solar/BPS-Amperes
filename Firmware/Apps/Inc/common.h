#ifndef COMMON_H
#define COMMON_H
/**
 * Common Functionality
 */

#include "stm32xx_hal.h"

/**
 * @brief Shared error hanlder
 */
void error_handler();

/**
 * @brief Shared success hanlder
 */
void success_handler();

/**
  * @brief System Clock Configuration: 80 MHz
  * @retval None
  */
void SystemClock_Config(void);

#endif