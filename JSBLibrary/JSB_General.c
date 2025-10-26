#include "string.h"
//
#include "stm32f7xx_hal.h"
#include "stm32f7xx_nucleo_144.h"
//
#include "JSB_STM32F767ZINucleo144.h"
#include "JSB_General.h"
//
#if JSBGeneral_Sockets
#include <sys/time.h>
#include "lwip/sys.h"
#include "lwip/sockets.h"
#endif

uint32_t min(uint32_t i_A, uint32_t i_B)
{
  return i_A < i_B ? i_A : i_B;
}

uint32_t max(uint32_t i_A, uint32_t i_B)
{
  return i_A > i_B ? i_A : i_B;
}

uint32_t clamp(uint32_t i_Value, uint32_t i_Min, uint32_t i_Max)
{
  if (i_Value < i_Min)
    i_Value = i_Min;
  if (i_Value > i_Max)
    i_Value = i_Max;
  return i_Value;
}

void JSB_ErrorHandler()
{
//  while (1)
//  {
//    for (int Index = 0; Index < 3; ++Index)
//    {
//      BSP_LED_On(LED1);
//      BSP_LED_On(LED2);
//      BSP_LED_On(LED3);
//      HAL_Delay(100);
//      BSP_LED_Off(LED1);
//      BSP_LED_Off(LED2);
//      BSP_LED_Off(LED3);
//      HAL_Delay(100);
//    }
//    HAL_Delay(1000);
//  }

// JSB: Version that doesn't use HAL_Delay/osDelay as these functions fail during error conditions.
// JSB: The flash rate will depend on the clock speeds, however.
  while (1)
  {
    for (int Index = 0; Index < 3; ++Index)
    {
      for (int Duration = 0; Duration < 1000000; ++Duration)
      {
        BSP_LED_On(LED1);
        BSP_LED_On(LED2);
        BSP_LED_On(LED3);
      }

      for (int Duration = 0; Duration < 1000000; ++Duration)
      {
        BSP_LED_Off(LED1);
        BSP_LED_Off(LED2);
        BSP_LED_Off(LED3);
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Sockets:

#if JSBGeneral_Sockets

int JSB_Sockets_Connect(const char *IP, u16_t Port, int SocketType)
{
  int fdSocket;
  struct sockaddr_in RemoteAddress;

  errno = 0;
  if ((fdSocket = socket(PF_INET, SocketType, 0)) < 0)
  {
    printf("Sockets: Failed to create socket. Socket error: %d.\r\n", errno);
    return -1;
  }

  memset(&RemoteAddress, 0, sizeof(RemoteAddress));
  RemoteAddress.sin_family = AF_INET;
  RemoteAddress.sin_addr.s_addr = inet_addr(IP);
  RemoteAddress.sin_port = htons(Port);

  errno = 0;
  if (connect(fdSocket, (struct sockaddr *)&RemoteAddress, sizeof(RemoteAddress)) < 0)
  {
    printf("Sockets: Failed to connect. Socket error: %d.\r\n", errno);
    close(fdSocket);
    return -1;
  }

  return fdSocket;
}

#endif // JSBGeneral_Sockets

///////////////////////////////////////////////////////////////////////////////
