///////////////////////////////////////////////////////////////////////////////
// JSB:
//
// Note: This project is to demonstrate GPIO usage, so the provided BSP_LED functions are not used.
///////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "stm32f7xx_nucleo_144.h"
#include "JSB_STM32F767ZINucleo144.h"

int main(void)
{
  JSB_EnableCPUCache();
  HAL_Init();
  JSB_InitSystemClock_216MHz();
  JSB_GPIO_EnableClocks();

  // Initialize GPIO ports for LEDs:
  JSB_GPIO_InitPin(LED1_GPIO_PORT, LED1_PIN, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH, 0); // Or use: BSP_LED_Init(LED1);
  JSB_GPIO_InitPin(LED2_GPIO_PORT, LED2_PIN, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH, 0); // Or use: BSP_LED_Init(LED2);
  JSB_GPIO_InitPin(LED3_GPIO_PORT, LED3_PIN, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH, 0); // Or use: BSP_LED_Init(LED3);

  // Set initialize LED state:
  HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_RESET); // Or use: BSP_LED_Off(LED1);
  HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, GPIO_PIN_RESET); // Or use: BSP_LED_Off(LED2);
  HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_PIN, GPIO_PIN_SET); // Or use: BSP_LED_On(LED3);

  int ToggleIndex=0;
  while (1)
  {
    HAL_Delay(1000);

    HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_PIN); // Or use: BSP_LED_Off(LED1);

    HAL_Delay(1000);

    HAL_GPIO_TogglePin(LED2_GPIO_PORT, LED2_PIN); // Or use: BSP_LED_Toggle(LED2);
    HAL_GPIO_TogglePin(LED3_GPIO_PORT, LED3_PIN); // Or use: BSP_LED_Toggle(LED3);

    ++ToggleIndex;
  }
}
