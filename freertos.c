//
// FreeRTOS setup
//

/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "cmsis_os.h"

/* USER CODE BEGIN Includes */
#include "platform.h"
#include "tasks/sched_queue.h"
#include "stacksmon.h"
/* USER CODE END Includes */

/* Variables -----------------------------------------------------------------*/

osThreadId tskMainHandle;
uint32_t mainTaskStack[ TSK_STACK_MAIN ];
osStaticThreadDef_t mainTaskControlBlock;

osThreadId tskMsgJobHandle;
uint32_t msgJobQueTaskStack[ TSK_STACK_MSG ];
osStaticThreadDef_t msgJobQueTaskControlBlock;

osMessageQId queMsgJobHandle;
uint8_t msgJobQueBuffer[ RX_QUE_CAPACITY * sizeof( struct rx_sched_combined_que_item ) ];
osStaticMessageQDef_t msgJobQueControlBlock;

osMutexId mutTinyFrameTxHandle;
osStaticMutexDef_t mutTinyFrameTxControlBlock;

osSemaphoreId semVcomTxReadyHandle;
osStaticSemaphoreDef_t semVcomTxReadyControlBlock;

osMutexId mutScratchBufferHandle;
osStaticMutexDef_t mutScratchBufferControlBlock;

/* USER CODE BEGIN Variables */

/* USER CODE END Variables */

/* Function prototypes -------------------------------------------------------*/
void TaskMain(void const * argument);
extern void TaskMsgJob(void const *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* Hook prototypes */
void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName);

/* USER CODE BEGIN 4 */
__weak void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
}
/* USER CODE END 4 */

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[TSK_STACK_IDLE];
  
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = TSK_STACK_IDLE;
  /* place for user code */
}                   
/* USER CODE END GET_IDLE_TASK_MEMORY */


/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xTimersTaskTCBBuffer;
static StackType_t xTimersStack[TSK_STACK_TIMERS];

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimersTaskTCBBuffer, StackType_t **ppxTimersTaskStackBuffer, uint32_t *pulTimersTaskStackSize )
{
  *ppxTimersTaskTCBBuffer = &xTimersTaskTCBBuffer;
  *ppxTimersTaskStackBuffer = &xTimersStack[0];
  *pulTimersTaskStackSize = TSK_STACK_TIMERS;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */


/* Init FreeRTOS */

void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  stackmon_register("Main", mainTaskStack, sizeof(mainTaskStack));
  stackmon_register("Job+Msg", msgJobQueTaskStack, sizeof(msgJobQueTaskStack));
  stackmon_register("Idle", xIdleStack, sizeof(xIdleStack));
  stackmon_register("Timers", xTimersStack, sizeof(xTimersStack));
  /* USER CODE END Init */

  /* Create the mutex(es) */
  /* definition and creation of mutTinyFrameTx */
  osMutexStaticDef(mutTinyFrameTx, &mutTinyFrameTxControlBlock);
  mutTinyFrameTxHandle = osMutexCreate(osMutex(mutTinyFrameTx));

  osMutexStaticDef(mutScratchBuffer, &mutScratchBufferControlBlock);
  mutScratchBufferHandle = osMutexCreate(osMutex(mutScratchBuffer));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of semVcomTxReady */
  osSemaphoreStaticDef(semVcomTxReady, &semVcomTxReadyControlBlock);
  semVcomTxReadyHandle = osSemaphoreCreate(osSemaphore(semVcomTxReady), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  xSemaphoreGive(semVcomTxReadyHandle);
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the thread(s) */
  /* definition and creation of tskMain */
  osThreadStaticDef(tskMain, TaskMain, osPriorityHigh, 0, TSK_STACK_MAIN, mainTaskStack, &mainTaskControlBlock);
  tskMainHandle = osThreadCreate(osThread(tskMain), NULL);

  /* definition and creation of TaskMessaging */
  osThreadStaticDef(tskMsg, TaskMsgJob, osPriorityNormal, 0, TSK_STACK_MSG, msgJobQueTaskStack, &msgJobQueTaskControlBlock);
  tskMsgJobHandle = osThreadCreate(osThread(tskMsg), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the queue(s) */

  /* definition and creation of queRxData */
  osMessageQStaticDef(queMsgJob, RX_QUE_CAPACITY, struct rx_sched_combined_que_item, msgJobQueBuffer, &msgJobQueControlBlock);
  queMsgJobHandle = osMessageCreate(osMessageQ(queMsgJob), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
}

/* TaskMain function */
__weak void TaskMain(void const * argument)
{

  /* USER CODE BEGIN TaskMain */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END TaskMain */
}

/* USER CODE BEGIN Application */
     
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
