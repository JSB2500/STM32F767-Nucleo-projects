///////////////////////////////////////////////////////////////////////////////
// JSB:
//
// UART debugging info:
// 1) Emitted from UART3 at 115200 baud, 8 bit, no parity, no handshake. Can be received via ST-Link VCP or standalone VCP connected to RX pin of connector CN5.
// 2) To output info, press and hold the blud user button. May take up to 5 seconds for first output.
// 3) To enable lwIP debugging, see end of "lwipopts.h".
//
// LED debugging info:
// 1) All three lights flashing indicates fatal error.
// 2) Red light indicates no IP assigned by DHCP server. [Accurate???]
// 3) Green light indicates IP assigned by DHCP server. [Accurate???]
//
// Door:
// 1) Proxy for remote door server (for testing) in Test folder. Requires Python 3.
//
///////////////////////////////////////////////////////////////////////////////

#include <doorlock.h>
#include "stdarg.h"
#include "string.h"
#include "JSB_General.h"
#include "JSB_STM32F767ZINucleo144.h"
//
#include "main.h"  // Include before lwIP as it contains "USE_DHCP" define.
//
#include "cmsis_os.h"
//
#include <sys/time.h>
#include "ethernetif.h"
#include "app_ethernet.h"
#include "lwipopts.h"
#include "lwip/tcpip.h"
#include "lwip/stats.h"
//
#include "task.h"
//
#include "httpserver-netconn.h"
#include "httpserver-socket.h"
#include "generalserver.h"

#if (configCHECK_FOR_STACK_OVERFLOW > 0)
void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName)
{
  JSB_ErrorHandler();
}
#endif

void Report()
{
  char S[1024];

  if (JSB_GetUserButtonPressed())
  {
    printf("----------------------------------------\r\n");
    vTaskList(S); // JSB: The stack column gives the unused stack space in 32 bit words.
    printf(S);
    printf("B=Blocked R=Ready D=Deleted S=Suspended\r\n");
    printf("----------------------------------------\r\n");

#if LWIP_STATS_DISPLAY
    stats_display();
    printf("----------------------------------------\r\n");
#endif
  }
}

void ReportingThread(void * arg)
{
  while (1)
  {
    Report();
    osDelay(5000);
  }
}

void CreateReportingThread()
{
  if (!sys_thread_new("Reporting", ReportingThread, NULL, 640, osPriorityNormal))
  {
    printf("Failed to create report thread info thread.\r\n");
    JSB_ErrorHandler();
  }
}

static struct netif gnetif;

static void Netif_Config(void)
{
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;

#ifdef USE_DHCP
  ip_addr_set_zero_ip4(&ipaddr);
  ip_addr_set_zero_ip4(&netmask);
  ip_addr_set_zero_ip4(&gw);
#else
  IP_ADDR4(&ipaddr,IP_ADDR0,IP_ADDR1,IP_ADDR2,IP_ADDR3);
  IP_ADDR4(&netmask,NETMASK_ADDR0,NETMASK_ADDR1,NETMASK_ADDR2,NETMASK_ADDR3);
  IP_ADDR4(&gw,GW_ADDR0,GW_ADDR1,GW_ADDR2,GW_ADDR3);
#endif

  /* Add the network interface */
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

  /*  Registers the default network interface. */
  netif_set_default(&gnetif);

  if (netif_is_link_up(&gnetif))
  {
    /* When the netif is fully configured this function must be called.*/
    netif_set_up(&gnetif);
  }
  else
  {
    /* When the netif link is down this function must be called */
    netif_set_down(&gnetif);
  }
}

/**
 * @brief  Configure the MPU attributes as Device for Ethernet Descriptors in the SRAM1.
 * @note   The Base Address is 0x20020000 since this memory interface is the AXI.
 *         The Configured Region Size is 256B (size of Rx and Tx ETH descriptors)
 *
 * @param  None
 * @retval None
 */
static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

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

  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

static void CreateDHCPThread()
{
  osThreadDef(DHCP, DHCP_thread, osPriorityBelowNormal, 0, 256);

  if (!osThreadCreate(osThread(DHCP), &gnetif))
  {
    printf("Failed to create DHCP thread.\r\n");
    JSB_ErrorHandler();
  }
}

static void InitializationThread(void * argument)
{
  tcpip_init(NULL, NULL);
  Netif_Config();
  User_notification(&gnetif);

#ifdef USE_DHCP
  CreateDHCPThread();
#endif

#ifdef EnableReporting
  printf("Reporting enabled.\r\n");
  CreateReportingThread();
#endif

#ifdef UseSocketAPIForHTTPServer
  printf("Using socket API for HTTP Server.\r\n");
  CreateHTTPServerThread_Socket();
#else
  printf("Using netconn API for HTTP Server.\r\n");
  CreateHTTPServerThread_NetConn();
#endif

#ifdef EnableDoorLockClient
  printf("Door client enabled.\r\n");
  CreateDoorLockClientThread();
#endif

  CreateGeneralServerListenerThread();

  for (;;)
  {
    /* Delete the Init thread */
    osThreadTerminate(NULL); // !!!JSB: Check that this is not consuming the CPU.
  }
}

static void CreateInitializationThread()
{
  if (!sys_thread_new("Initialization", InitializationThread, NULL, 512, osPriorityNormal))
  {
    printf("Failed to create initialization thread.\r\n");
    JSB_ErrorHandler();
  }
}

int main(void)
{
  MPU_Config();
  JSB_EnableCPUCache();
  HAL_Init();
  JSB_GPIO_EnableClocks();

  // Setup before setting the clock to facilitate error reporting.
  BSP_LED_Init(LED1);
  BSP_LED_Init(LED2);
  BSP_LED_Init(LED3);

  JSB_InitSystemClock_200MHz();
  JSB_InitDebuggingUART();
  JSB_InitUserButton();

  CreateInitializationThread();

  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  JSB_ErrorHandler();
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
