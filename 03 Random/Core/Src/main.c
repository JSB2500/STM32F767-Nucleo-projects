#include "stdio.h"
#include "stdlib.h"
#include "main.h"
#include "JSB_STM32F767_Nucleo144.h"
#include "JSB_General.h"

RNG_HandleTypeDef RngHandle;

int main(void)
{
  JSB_EnableCPUCache();
  HAL_Init();
  JSB_GPIO_EnableClocks();

  // Setup before setting the clock to facilitate error reporting.
  BSP_LED_Init(LED1);
  BSP_LED_Init(LED2);
  BSP_LED_Init(LED3);

  JSB_InitSystemClock_216MHz();

  JSB_InitDebuggingUART();
  printf("Printf() redirected to USART3.\r\n");

  RngHandle.Instance = RNG;
  if (HAL_RNG_DeInit(&RngHandle) != HAL_OK) // Requires HAL_RNG_MspInit().
    JSB_ErrorHandler();
  if (HAL_RNG_Init(&RngHandle) != HAL_OK) // Requires HAL_RNG_MspDeInit().
    JSB_ErrorHandler();

  BSP_LED_Toggle(LED2);

  int C_R, HAL_R = 0;
  double C_RD, HAL_RD;
  double C_Total = 0.0, HAL_Total = 0.0;
  double C_Mean, HAL_Mean;
  int NumItems = 0;
  while (1)
  {
    C_R = random();
    C_RD = ((double) C_R) / ((double) RAND_MAX);
    C_Total += C_RD;
    C_Mean = C_Total / NumItems;

    if (HAL_RNG_GenerateRandomNumber(&RngHandle, (uint32_t *) &HAL_R) != HAL_OK)
      JSB_ErrorHandler();
    HAL_RD = ((double) HAL_R) / ((double) 0xFFFFFFFF);
    HAL_Total += HAL_RD;
    HAL_Mean = HAL_Total / NumItems;

    printf("Random number: C: %u %f %f HAL: %u %f %f.\r\n", C_R, C_RD, C_Mean, HAL_R, HAL_RD, HAL_Mean); // Requires "-u _printf_float" in library flags.

    BSP_LED_Toggle(LED1);
    BSP_LED_Toggle(LED2);
    BSP_LED_Toggle(LED3);

    HAL_Delay(100);

    ++NumItems;
  }
}
