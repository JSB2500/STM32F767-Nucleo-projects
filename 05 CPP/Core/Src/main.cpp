#include "main.h"
#include "stm32f7xx_nucleo_144.h"
#include "JSB_STM32F767ZINucleo144.h"
#include "JSB_General.h"
#include "test.h"

int main(void)
{
  JSB_EnableCPUCache();
  HAL_Init();
  JSB_GPIO_EnableClocks();

  // Setup before setting the clock, to facilitate error reporting.
  BSP_LED_Init(LED1);
  BSP_LED_Init(LED2);
  BSP_LED_Init(LED3);

  JSB_InitSystemClock_216MHz();
  JSB_InitDebuggingUART();

  JSB_InitUserButton();

  Test();
}
