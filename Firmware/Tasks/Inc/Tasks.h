#ifndef TASKS_H
#define TASKS_H
#include "Amperes.h"

/* ADC Task */
#define ADC_TASK_PRIO           tskIDLE_PRIORITY + 3
#define ADC_TASK_STACK_SIZE     configMINIMAL_STACK_SIZE * 2
extern StaticTask_t             ADC_task_buffer;
extern StackType_t              ADC_task_stack[ADC_TASK_STACK_SIZE];

/* CAN Task */
#define CAN_TASK_PRIO           tskIDLE_PRIORITY + 2
#define CAN_TASK_STACK_SIZE     configMINIMAL_STACK_SIZE * 2
extern StaticTask_t             CAN_task_buffer;
extern StackType_t              CAN_task_stack[CAN_TASK_STACK_SIZE];

/* Init Task */
#define TASK_INIT_PRIO          tskIDLE_PRIORITY + 1
#define TASK_INIT_STACK_SIZE    configMINIMAL_STACK_SIZE * 4
extern StaticTask_t             Task_Init_Buffer;
extern StackType_t              Task_Init_Stack_Array[TASK_INIT_STACK_SIZE];

// TODO: SET STACK SIZE AND PRIORITIES

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
