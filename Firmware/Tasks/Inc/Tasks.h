#ifndef TASKS_H
#define TASKS_H
#include "Amperes.h"

/**
 * Amperes Task Period
 * - ADC LPF cutoff frequency is ~100 Hz
 * - Sample at around 10 times that: 1000 Hz (1ms period)
 */
#define AMPERES_TASK_PERIOD         pdMS_TO_TICKS(100)

/**
 * Amperes Task
 * - Combination of ADC and CAN task
 */
#define AMPERES_TASK_PRIO           tskIDLE_PRIORITY + 3
#define AMPERES_TASK_STACK_SIZE     configMINIMAL_STACK_SIZE * 2
extern StaticTask_t                 Amperes_Task_Buffer;
extern StackType_t                  Amperes_Task_Stack[AMPERES_TASK_STACK_SIZE];

/**
 * ADC Task
 * - Lower priority than CAN; run when CAN is finished
 * - Trigger ADC conversion and block until queue receives data
 */
#define ADC_TASK_PRIO           tskIDLE_PRIORITY + 2
#define ADC_TASK_STACK_SIZE     configMINIMAL_STACK_SIZE * 2
extern StaticTask_t             ADC_Task_Buffer;
extern StackType_t              ADC_Task_Stack[ADC_TASK_STACK_SIZE];

/**
 * CAN Task
 * - Highest priority
 * - Block on queue data from ADC Task and then send CAN message
 */
#define CAN_TASK_PRIO           tskIDLE_PRIORITY + 3
#define CAN_TASK_STACK_SIZE     configMINIMAL_STACK_SIZE * 2
extern StaticTask_t             CAN_Task_Buffer;
extern StackType_t              CAN_Task_Stack[CAN_TASK_STACK_SIZE];

/**
 * Init Task
 * - Lowest priority: initializes tasks and then deletes itself
 */
#define TASK_INIT_PRIO          tskIDLE_PRIORITY + 1
#define TASK_INIT_STACK_SIZE    configMINIMAL_STACK_SIZE * 4
extern StaticTask_t             Task_Init_Buffer;
extern StackType_t              Task_Init_Stack[TASK_INIT_STACK_SIZE];

// TODO: SET STACK SIZE AND PRIORITIES

/**
 * @brief   Amperes Task
 * @n
 * 1) ADC: Starts ADC conversion, then blocks until data is available.
 * @n
 * 2) Convert ADC value to current measurement.
 * @n
 * 3) CAN: Send current measurement over CAN
 * @n
 * Period is set to TODO
 */
void Amperes_Task(void *pvParameters);

/**
 * @brief   ADC Task. 
 * @n
 * - Sleeps until ADC conversion is done and data 
 *  is available in adc_queue; then converts ADC
 *  data into current measurement. 
 * @n
 * - Packs ADC value and current measurement into 
 *  AmperesMsg_t message, which is sent via can_queue to CAN task. 
 * @n
 * - Rate is set by Timer6 prompting ADC conversion 
 *  (which is then stored in adc_queue).
 * @retval  none
 */
void ADC_Task(void *pvParameters);

/**
 * @brief   CAN Task. 
 * @n 
 * - Sleeps until AmperesMsg is available in can_queue and sends data over CAN. 
 * @n 
 * - Rate is set by ADC_Task sending to can_queue.
 * @retval  none
 */
void CAN_Task(void *pvParameters);

/**
 * @brief Initializes all tasks (ADC and CAN).
 * @retval none
 */
void Task_Init();

#endif
