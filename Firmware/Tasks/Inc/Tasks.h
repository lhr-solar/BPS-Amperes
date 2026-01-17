#ifndef TASKS_H
#define TASKS_H
#include "Amperes.h"

/* Task Generation Macro 
// #define FOR_EACH_TASK(TASK, PRIO, STACK_SIZE)   \
//     extern StaticTask_t  TASK_##task_buffer;    \
//     extern StackType_t TASK_##task_stack[STACK_SIZE];   \
//     #define TASK_
*/

/* Tasks */
extern StaticTask_t ADC_task_buffer;
extern StackType_t  ADC_task_stack[configMINIMAL_STACK_SIZE];
#define ADC_PRIO    1
#define ADC_DELAY   pdMS_TO_TICKS(200)     // TODO

/**
 * @brief ADC Task: TODO
 * @retval none
 */
void ADC_Task(void *pvParameters);
void CAN_Task(void *pvParameters);

/* Init Task */
#define TASK_INIT_PRIO          tskIDLE_PRIORITY + 1
#define TASK_INIT_STACK_SIZE    configMINIMAL_STACK_SIZE

/**
 * @brief Initializes all tasks
 * @retval none
 */
void Task_Init();

#endif
