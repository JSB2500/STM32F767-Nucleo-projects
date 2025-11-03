#include "stdio.h"
#include "string.h"
#include "main.h"
#include "stm32f7xx_nucleo_144.h"
#include "JSB_STM32F767ZINucleo144.h"
#include "JSB_General.h"

#define BrightnessRange (1000U)

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *pTimerHandle)
{
  if (pTimerHandle->Instance == TIM2)
  {
    __HAL_RCC_TIM2_CLK_ENABLE();

    // Timer 2 port pins:
    // => Channel 1: PA.5
    // => Channel 2: PB.3
    JSB_GPIO_InitPin(GPIOA, GPIO_PIN_5, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_HIGH, GPIO_AF1_TIM2);
    JSB_GPIO_InitPin(GPIOB, GPIO_PIN_3, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_HIGH, GPIO_AF1_TIM2);
  }

  if (pTimerHandle->Instance == TIM3)
  {
    __HAL_RCC_TIM3_CLK_ENABLE();

    // Timer 3 port pins:
    // => Channel 1: PB.1
    // => Channel 2: PB.2
    JSB_GPIO_InitPin(GPIOB, GPIO_PIN_0, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_HIGH, GPIO_AF2_TIM3); // Green LED (LED1).
//    JSB_GPIO_InitPin(GPIOB, GPIO_PIN_1, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_HIGH, GPIO_AF2_TIM3);
  }
}

void InitializeTimer(TIM_HandleTypeDef *pTimerHandle, TIM_TypeDef *pTimer, uint32_t i_ChannelA, uint32_t i_ChannelB)
{
  memset(pTimerHandle, 0, sizeof(TIM_HandleTypeDef));
  pTimerHandle->Instance = pTimer;

  // Initialize timer:
  if (JSB_Timer_InitPWM(pTimerHandle, TIM_COUNTERMODE_UP, 20, 2 * BrightnessRange) != HAL_OK)
  {
    printf("Failed to initialize timer.");
    JSB_ErrorHandler();
  }

  // Configure timer PWM channels:
  if ((JSB_Timer_ConfigureChannel(pTimerHandle, i_ChannelA, TIM_OCMODE_PWM1, TIM_OCPOLARITY_HIGH, 0U) != HAL_OK)
    || (JSB_Timer_ConfigureChannel(pTimerHandle, i_ChannelB, TIM_OCMODE_PWM1, TIM_OCPOLARITY_LOW, 0U) != HAL_OK))
  {
    printf("Failed to configure timer.");
    JSB_ErrorHandler();
  }

  // Start PWM signals generation:
  if ((HAL_TIM_PWM_Start(pTimerHandle, i_ChannelA) != HAL_OK)
    || (HAL_TIM_PWM_Start(pTimerHandle, i_ChannelB) != HAL_OK))
  {
    printf("Failed to start timer.");
    JSB_ErrorHandler();
  }
}

void SetBrightness(TIM_HandleTypeDef *pTimerHandle, uint32_t i_PhaseABrightness, uint32_t i_PhaseBBrightness, uint32_t i_Range, uint32_t i_ChannelA, uint32_t i_ChannelB)
{
  i_PhaseABrightness = Clamp_uint32(i_PhaseABrightness, 0, i_Range);
  i_PhaseBBrightness = Clamp_uint32(i_PhaseBBrightness, 0, i_Range);

  JSB_Timer_SetCaptureCompareRegister(pTimerHandle, i_ChannelA, i_PhaseABrightness);
  JSB_Timer_SetCaptureCompareRegister(pTimerHandle, i_ChannelB, 2 * i_Range - i_PhaseBBrightness);
}

int main(void)
// Notes:
// 1) Ensure that the resulting frequency is not too high for the light driver amplifier.
// 2) Timer 2 source is 2 * APB1 which is typically 108 MHz for 216 MHz SYSCLK (dependent on RCC_HCLK_DIV4).
{
  TIM_HandleTypeDef Timer2Handle, Timer3Handle;
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

  PhaseABrightness = 750;
  PhaseBBrightness = 250;

  InitializeTimer(&Timer2Handle, TIM2, TIM_CHANNEL_1, TIM_CHANNEL_2);
  InitializeTimer(&Timer3Handle, TIM3, TIM_CHANNEL_3, TIM_CHANNEL_4);

  SetBrightness(&Timer2Handle, PhaseABrightness, PhaseBBrightness, BrightnessRange, TIM_CHANNEL_1, TIM_CHANNEL_2);
  SetBrightness(&Timer3Handle, PhaseABrightness, PhaseBBrightness, BrightnessRange, TIM_CHANNEL_3, TIM_CHANNEL_4);

  while (1)
  {
    if (JSB_GetUserButtonPressed())
    {
      PhaseABrightness += 5;
      if (PhaseABrightness > BrightnessRange)
        PhaseABrightness = 0;

      PhaseBBrightness += 1;
      if (PhaseBBrightness > BrightnessRange)
        PhaseBBrightness = 0;

      SetBrightness(&Timer2Handle, PhaseABrightness, PhaseBBrightness, BrightnessRange, TIM_CHANNEL_1, TIM_CHANNEL_2);
      SetBrightness(&Timer3Handle, PhaseABrightness, PhaseBBrightness, BrightnessRange, TIM_CHANNEL_3, TIM_CHANNEL_4);
    }

    HAL_Delay(10);
  }
}
