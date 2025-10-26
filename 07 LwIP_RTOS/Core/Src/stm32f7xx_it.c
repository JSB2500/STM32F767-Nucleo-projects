#include "stm32f7xx_it.h"
#include "JSB_General.h"
#include "main.h"
#include "cmsis_os.h"

extern ETH_HandleTypeDef EthHandle;

// *** Cortex exception handlers ***

void NMI_Handler(void)
{
  JSB_ErrorHandler();
}

void HardFault_Handler(void)
{
  JSB_ErrorHandler();
}

void MemManage_Handler(void)
{
  JSB_ErrorHandler();
}

void BusFault_Handler(void)
{
  JSB_ErrorHandler();
}

void UsageFault_Handler(void)
{
  JSB_ErrorHandler();
}

void DebugMon_Handler(void)
{
}

void SysTick_Handler(void)
{
  osSystickHandler();
}

// *** STM32F7xx Peripherals Interrupt Handlers ***

void ETH_IRQHandler(void)
{
  HAL_ETH_IRQHandler(&EthHandle);
}

//void PPP_IRQHandler(void)
//{
//}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
