/**
 ******************************************************************************
 * @file    LwIP/LwIP_HTTP_Server_Socket_RTOS/Src/httpserver-socket.c
 * @author  MCD Application Team
 * @version V1.2.0
 * @date    30-December-2016
 * @brief   Basic http server implementation using LwIP socket API
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics International N.V.
 * All rights reserved.</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions are met:
 *
 * 1. Redistribution of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of other
 *    contributors to this software may be used to endorse or promote products
 *    derived from this software without specific written permission.
 * 4. This software, including modifications and/or derivative works of this
 *    software, must execute solely and exclusively on microcontroller or
 *    microprocessor devices manufactured by or for STMicroelectronics.
 * 5. Redistribution and use of this software other than as permitted under
 *    this license is void and will automatically terminate your rights under
 *    this license.
 *
 * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
 * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
 * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

#include "string.h"
#include "JSB_STM32F767ZINucleo144.h"
#include "JSB_General.h"
#include "main.h" // For Error_Handler().
#include "httpserver-socket.h"
#include "httpserver-data.h"
//
#include "cmsis_os.h"
//
#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "lwip/apps/fs.h"

#include <stdio.h>

#define WEBSERVER_THREAD_PRIORITY    ( osPriorityAboveNormal )

static u32_t NumPageHits = 0;

static void ServeDynamicWebPage(int conn)
{
  #define PageBody_MaxLength 1024

  portCHAR PageBody[PageBody_MaxLength];
  portCHAR strNumPageHits[10];

  memset(PageBody, 0, PageBody_MaxLength);

  // Add page hits:
  ++NumPageHits;
  sprintf(strNumPageHits, "%d", (int)NumPageHits);
  strcat(PageBody, strNumPageHits);

  // Add table of RTOS info:
  strcat((char *)PageBody, "<pre><br>Name          State  Priority  Stack   Num");
  strcat((char *)PageBody, "<br>---------------------------------------------<br>");
  osThreadList((unsigned char *)(PageBody + strlen(PageBody))); // Allow 2048
  strcat((char *)PageBody, "<br><br>---------------------------------------------");
  strcat((char *)PageBody, "<br>B=Blocked R=Ready D=Deleted S=Suspended<br>");

  // Send the dynamically generated page:
  write(conn, DynamicWebPageHeader, strlen((char* )DynamicWebPageHeader));
  write(conn, PageBody, strlen(PageBody));
}

static void HTTPServerServe(int conn)
{
  const int buflen = 1500;
  //
  int ret;
  struct fs_file file;
  unsigned char recv_buffer[buflen];

  ret = read(conn, recv_buffer, buflen);
  if (ret < 0)
    return;

  if (strncmp((char *)recv_buffer, "GET /STM32F7xx_files/ST.gif", 27) == 0)
  {
    fs_open(&file, "/STM32F7xx_files/ST.gif");
    write(conn, (const unsigned char* )(file.data), (size_t )file.len);
    fs_close(&file);
  }
  else if (strncmp((char *)recv_buffer, "GET /STM32F7xx_files/stm32.jpg", 30) == 0)
  {
    fs_open(&file, "/STM32F7xx_files/stm32.jpg");
    write(conn, (const unsigned char* )(file.data), (size_t )file.len);
    fs_close(&file);
  }
  else if (strncmp((char *)recv_buffer, "GET /STM32F7xx_files/logo.jpg", 29) == 0)
  {
    fs_open(&file, "/STM32F7xx_files/logo.jpg");
    write(conn, (const unsigned char* )(file.data), (size_t )file.len);
    fs_close(&file);
  }
  else if (strncmp((char *)recv_buffer, "GET /STM32F7xxTASKS.html", 24) == 0)
  {
    ServeDynamicWebPage(conn);
  }
  else if ((strncmp((char *)recv_buffer, "GET / ", 6) == 0) || (strncmp((char *)recv_buffer, "GET /STM32F7xx.html", 19) == 0))
  {
    fs_open(&file, "/STM32F7xx.html");
    write(conn, (const unsigned char* )(file.data), (size_t )file.len);
    fs_close(&file);
  }
  else
  {
    fs_open(&file, "/404.html");
    write(conn, (const unsigned char* )(file.data), (size_t )file.len);
    fs_close(&file);
  }

  close(conn);
}

static void HTTPServerThread(void *arg)
{
  int fdListeningSocket, fdConnection;
  socklen_t sizeof_RemoteAddress;
  struct sockaddr_in LocalAddress;
  struct sockaddr RemoteAddress;

  if ((fdListeningSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("HTTP server: Failed to create listening socket.\r\n");
    JSB_ErrorHandler();
  }

  /* Bind to interface */
  memset(&LocalAddress, 0, sizeof(LocalAddress)); // JSB: Ensures that the unused parts of the structure are zeroed.
  LocalAddress.sin_family = AF_INET;
  LocalAddress.sin_addr.s_addr = htonl(INADDR_ANY); // Any interface.
  LocalAddress.sin_port = htons(80); // Port number.
  if (bind(fdListeningSocket, (struct sockaddr *)&LocalAddress, sizeof (LocalAddress)) < 0)
  {
    printf("HTTP server: Failed to bind.\r\n");
    JSB_ErrorHandler();
  }

  /* Listen for incoming connections */
  listen(fdListeningSocket, 5);

  sizeof_RemoteAddress = sizeof(RemoteAddress);

  while (1)
  {
    fdConnection = accept(fdListeningSocket, &RemoteAddress, &sizeof_RemoteAddress);
    HTTPServerServe(fdConnection);
  }
}

void CreateHTTPServerThread_Socket()
{
  if (!sys_thread_new("HTTPServer", HTTPServerThread, NULL, 1280, WEBSERVER_THREAD_PRIORITY))
  {
    printf("Failed to create HTTP server thread\r\n");
    JSB_ErrorHandler();
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/











