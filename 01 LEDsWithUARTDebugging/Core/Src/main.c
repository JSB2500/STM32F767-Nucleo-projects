#include "stdio.h"
#include "main.h"
#include "JSB_STM32F767ZINucleo144.h"
#include "stm32f7xx_nucleo_144.h"

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

  BSP_LED_Toggle(LED2);
	
  int ToggleIndex=0;
  while (1)
  {
    printf("Toggle %d\r\n",ToggleIndex);

    BSP_LED_Toggle(LED1);
    BSP_LED_Toggle(LED2);
    BSP_LED_Toggle(LED3);

    HAL_Delay(1000);

    ++ToggleIndex;
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
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
