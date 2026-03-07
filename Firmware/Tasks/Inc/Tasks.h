#pragma once
#include "AmperesCAN.h"
#include "AmperesADC.h"
#include "common.h"

/** ================================================================
 *  Task Parameters: priority, stack size, static TCB buffer, stack arary
 * ================================================================ */

/**
 * Amperes Task
 * - Highest priority
 */
#define AMPERES_TASK_PRIO           tskIDLE_PRIORITY + 3
#define AMPERES_TASK_STACK_SIZE     configMINIMAL_STACK_SIZE * 2
extern StaticTask_t                 Amperes_Task_Buffer;
extern StackType_t                  Amperes_Task_Stack[AMPERES_TASK_STACK_SIZE];

/**
 * Init Task
 * - Lowest priority: initializes tasks and then deletes itself
 */
#define TASK_INIT_PRIO              tskIDLE_PRIORITY + 1
#define TASK_INIT_STACK_SIZE        configMINIMAL_STACK_SIZE * 4
extern StaticTask_t                 Task_Init_Buffer;
extern StackType_t                  Task_Init_Stack[TASK_INIT_STACK_SIZE];

/**
 * Amperes Task Period
 * - ADC LPF cutoff frequency is ~100 Hz
 * - Sample at around 10 times that: 1000 Hz (1ms period)
 */
#define AMPERES_TASK_PERIOD         pdMS_TO_TICKS(1)


/** ================================================================
 *  Tasks
 * ================================================================ */

/**
 * @brief   Amperes Task
 * @n
 * 1) ADC: Starts ADC conversion, then blocks until data is available in queue
 *         (queue goes from empty -> has a value). Must reset queue before.
 * @n
 * 2) Convert ADC value to current measurement.
 * @n
 * 3) CAN: Send current measurement over CAN.
 * @n
 * ADC conversion happens at 1000 Hz.
 * Values are averaged and sent over CAN every 100 Hz (10 sample avg).
 */
void Amperes_Task(void *pvParameters);

/**
 * @brief Initializes Amperes hardware and task,
 *        then deletes itself.
 * @retval none
 */
void Task_Init();