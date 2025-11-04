#include "main.h"
#include "JSB_STM32F767_Nucleo144.h"
#include "JSB_General.h"
#include "cmsis_os.h"

static void MPU_Config(void);

// *** Begin RTOS ***

void Thread1_Thread(void const * Argument);
void Thread2_Thread(void const * Argument);
void Thread3_Thread(void const * Argument);
//
void Timer_Callback(void const *Argument);

typedef uint32_t MessageData_t;
osPoolDef(MemoryPool, 16, MessageData_t);
osPoolId MemoryPool;
osMessageQDef(MessageQueue, 16, MessageData_t);
osMessageQId MessageQueue;
osTimerDef(Timer, Timer_Callback);
osTimerId Timer;

// *** End RTOS ***

static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU attributes as Device for Ethernet Descriptors in the SRAM */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x20020000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256B;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

int main()
{
  MPU_Config();
  JSB_EnableCPUCache();
  HAL_Init();
  JSB_GPIO_EnableClocks();

  // Setup before setting the clock to facilitate error reporting.
  BSP_LED_Init(LED1);
  BSP_LED_Init(LED2);
  BSP_LED_Init(LED3);

  JSB_InitSystemClock_216MHz();
  JSB_InitDebuggingUART();
  JSB_InitUserButton();

  MemoryPool = osPoolCreate(osPool(MemoryPool));
  MessageQueue = osMessageCreate(osMessageQ(MessageQueue), NULL);

  osThreadDef(Thread1, Thread1_Thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 2);
  osThreadCreate(osThread(Thread1), NULL);

  osThreadDef(Thread2, Thread2_Thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 2);
  osThreadCreate(osThread(Thread2), NULL);

  osThreadDef(Thread3, Thread3_Thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 2);
  osThreadCreate(osThread(Thread3), NULL);

  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  for (;;)
    ;
}

/* Delete the Init Thread */
//    osThreadTerminate(NULL);
/* */

void Timer_Callback(void const *Argument)
// Warning: Only appears to get called if the associated thread is in osDelay(). Calling osThreadYield() from the thread is not sufficient.
{
  printf("Timer callback\r\n");
}

void Thread1_Thread(void const * argument)
{
  MessageData_t *pMessageData;
  int ButtonPressed;

  Timer = osTimerCreate(osTimer(Timer), osTimerPeriodic, NULL);
  osTimerStart(Timer, 5000);

  while (1)
  {
    printf("Thread 1\r\n");

    ButtonPressed = 0;
    while (JSB_GetUserButtonPressed())
    {
      if (!ButtonPressed)
      {
        ButtonPressed = 1;
        printf("Blue button pressed.\r\n");
        BSP_LED_On(LED1);
      }
    }

    if (ButtonPressed)
    {
      ButtonPressed = 0;
      printf("Blue button released.\r\n");

      pMessageData = osPoolAlloc(MemoryPool);
      *pMessageData = 1234;
      printf("Sending message data: %u\r\n", (unsigned int)*pMessageData);
      osMessagePut(MessageQueue, (uint32_t)pMessageData, osWaitForever);
    }

    BSP_LED_Toggle(LED1);
    osDelay(200);
  }
}

void Thread2_Thread(void const * argument)
{
  while (1)
  {
    printf("Thread 2\r\n");

    BSP_LED_Toggle(LED2);
    osDelay(500);
  }
}

void Thread3_Thread(void const * argument)
{
  osEvent Event;
  MessageData_t *pMessageData;

  while (1)
  {
    Event = osMessageGet(MessageQueue, osWaitForever);
    if (Event.status == osEventMessage)
    {
      pMessageData = Event.value.p;

      printf("Thread 3\r\n");

      printf("Received message data: %u\r\n", (unsigned int) (*pMessageData));
      osPoolFree(MemoryPool, pMessageData);

      BSP_LED_On(LED3);
      osDelay(1000);
      BSP_LED_Off(LED3);
    }
  }
}

#ifdef  USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
   ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif
