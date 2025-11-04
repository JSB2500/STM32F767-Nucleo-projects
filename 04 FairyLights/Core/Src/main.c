///////////////////////////////////////////////////////////////////////////////
// Notes:
// => Outputs:
//    => PWM: Timer 2 channel 1
//    => Direction: Timer 2 channel 3
// => Ensure that the operating frequency is not too high for the light driver amplifier.
// => Timer 2 source is 2 * APB1 which is typically 108 MHz for 216 MHz SYSCLK (dependent on RCC_HCLK_DIV4).
//
// Reference material:
// => en.DM00042534.pdf
///////////////////////////////////////////////////////////////////////////////

#include "stdio.h"
#include "string.h"
#include "main.h"
#include "stm32f7xx_nucleo_144.h"
#include "JSB_STM32F767_Nucleo144.h"
#include "JSB_General.h"

// Output frequency = ((SysClkFrequency / 2) / Prescale) / (2 * BrightnessRange)) = SysClkFrequency / (4 * Prescale * BrightnessRange)
// E.g. If SysClkFrequency = 216 MHz and Prescale = 20, then output frequency = 2700000 / BrightnessRange.
// If, further, BrightnessRange = 1000, then output frequency = 2700 Hz.
#define BrightnessRange (1000U)

TIM_HandleTypeDef TimerHandle;

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *pTimerHandle)
{
  if (pTimerHandle->Instance == TIM2)
  {
    __HAL_RCC_TIM2_CLK_ENABLE()
    ;

    // Timer 2 port pins:
    // => Channel 1: PA.5
    // => Channel 2: PB.3
    // => Channel 3: PB.10
    // => Channel 4: PB.11
    JSB_GPIO_InitPin(GPIOA, GPIO_PIN_5, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_HIGH, GPIO_AF1_TIM2);
    // JSB_GPIO_InitPin(GPIOB, GPIO_PIN_3, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_HIGH, GPIO_AF1_TIM2);
    JSB_GPIO_InitPin(GPIOB, GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_HIGH, GPIO_AF1_TIM2);
    // JSB_GPIO_InitPin(GPIOB, GPIO_PIN_11, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_HIGH, GPIO_AF1_TIM2);
  }
}

static void InitializeTimer()
{
  memset(&TimerHandle, 0, sizeof(TIM_HandleTypeDef));
  TimerHandle.Instance = TIM2;

  // Initialize timer:
  if (JSB_Timer_InitPWM(&TimerHandle, TIM_COUNTERMODE_UP, 20, 2 * BrightnessRange) != HAL_OK)
  {
    printf("Failed to initialize timer.");
    JSB_ErrorHandler();
  }

  // Configure timer PWM channels:
  if ((JSB_Timer_ConfigureChannel(&TimerHandle, TIM_CHANNEL_1, TIM_OCMODE_COMBINED_PWM2, TIM_OCPOLARITY_LOW, 0) != HAL_OK)
    || (JSB_Timer_ConfigureChannel(&TimerHandle, TIM_CHANNEL_2, TIM_OCMODE_PWM1, TIM_OCPOLARITY_LOW, 0) != HAL_OK)
    || (JSB_Timer_ConfigureChannel(&TimerHandle, TIM_CHANNEL_3, TIM_OCMODE_PWM1, TIM_OCPOLARITY_LOW, BrightnessRange) != HAL_OK))
  {
    printf("Failed to configure timer.");
    JSB_ErrorHandler();
  }

  // Start PWM signals generation:
  if ((HAL_TIM_PWM_Start(&TimerHandle, TIM_CHANNEL_1) != HAL_OK)
    || (HAL_TIM_PWM_Start(&TimerHandle, TIM_CHANNEL_2) != HAL_OK)
    || (HAL_TIM_PWM_Start(&TimerHandle, TIM_CHANNEL_3) != HAL_OK))
  {
    printf("Failed to start timer.");
    JSB_ErrorHandler();
  }
}

static void SetBrightness(uint32_t i_PhaseABrightness, uint32_t i_PhaseBBrightness)
{
  i_PhaseABrightness = Clamp_uint32(i_PhaseABrightness, 0, BrightnessRange);
  i_PhaseBBrightness = Clamp_uint32(i_PhaseBBrightness, 0, BrightnessRange);

  JSB_Timer_SetCaptureCompareRegister(&TimerHandle, TIM_CHANNEL_1, i_PhaseABrightness);
  JSB_Timer_SetCaptureCompareRegister(&TimerHandle, TIM_CHANNEL_2, 2 * BrightnessRange - i_PhaseBBrightness);
}

int main(void)
{
  uint32_t PhaseABrightness, PhaseBBrightness;

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

  PhaseABrightness = 500;
  PhaseBBrightness = 500;

  InitializeTimer();

  SetBrightness(PhaseABrightness, PhaseBBrightness);

  while (1)
  {
    PhaseABrightness += 1;
    if (PhaseABrightness > BrightnessRange)
      PhaseABrightness = 0;

    if (JSB_GetUserButtonPressed())
    {
      PhaseBBrightness += 1;
      if (PhaseBBrightness > BrightnessRange)
        PhaseBBrightness = 0;
    }

    SetBrightness(PhaseABrightness, PhaseBBrightness);

    HAL_Delay(10);
  }
}
