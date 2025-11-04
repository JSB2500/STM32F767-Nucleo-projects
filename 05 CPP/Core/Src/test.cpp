#include "stdio.h"
#include "stddef.h"
#include "test.h"
#include "stm32f7xx_nucleo_144.h"
#include "JSB_STM32F767_Nucleo144.h"
#include "JSB_General.h"

class Flash
{
public:
  uint32_t SleepTime;
  Flash(void);
  void Go();
};

Flash::Flash(void)
{
  SleepTime = 1000;
}

void Flash::Go()
{
  int ToggleIndex;

  printf("Press and hold the blue user button to throw an exception.\r\n");

  BSP_LED_On(LED3);

  try
  {
    while (1)
    {
      HAL_Delay(SleepTime);

      BSP_LED_Toggle(LED1);

      HAL_Delay(SleepTime);

      BSP_LED_Toggle(LED2);
      BSP_LED_Toggle(LED3);

      ++ToggleIndex;

      if (JSB_GetUserButtonPressed())
        throw 20;
   }
 }
 catch(int e)
 {
   printf("Exception thrown.\r\n");
   JSB_ErrorHandler();
 }
}

void Test()
{
  Flash *pA = NULL;

  pA = new Flash();
  pA->SleepTime = 100U;
  pA->Go();
}
