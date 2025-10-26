/**
 ******************************************************************************
 * @file    LwIP/LwIP_HTTP_Server_Netconn_RTOS/Src/httpser-netconn.c
 * @author  MCD Application Team
 * @version V1.1.0
 * @date    30-December-2016
 * @brief   Basic http server implementation using LwIP netconn API
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
#include "main.h" // For Error_Handler().
#include "httpserver-netconn.h"
#include "httpserver-data.h"
//
#include "cmsis_os.h"
//
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/apps/fs.h"
//
#include "JSB_General.h"

#include <stdio.h>

#define WEBSERVER_THREAD_PRIORITY    ( osPriorityAboveNormal )

static u32_t NumPageHits = 0;

static void ServeDynamicWebPage(struct netconn *conn)
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
  netconn_write(conn, DynamicWebPageHeader, strlen((char* )DynamicWebPageHeader), NETCONN_COPY);
  netconn_write(conn, PageBody, strlen(PageBody), NETCONN_COPY);
}

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
 * @brief serve tcp connection
 * @param conn: pointer on connection structure
 * @retval None
 */
static void http_server_serve(struct netconn *conn)
{
  struct netbuf *inbuf;
  err_t recv_err;
  char* buf;
  u16_t buflen;
  struct fs_file file;

  /* Read the data from the port, blocking if nothing yet there. 
   We assume the request (the part we care about) is in one netbuf */
  recv_err = netconn_recv(conn, &inbuf);

  if (recv_err == ERR_OK)
  {
    if (netconn_err(conn) == ERR_OK)
    {
      netbuf_data(inbuf, (void**)&buf, &buflen);

      /* Is this an HTTP GET command? (only check the first 5 chars, since
       there are other formats for GET, and we're keeping it very simple )*/
      if ((buflen >= 5) && (strncmp(buf, "GET /", 5) == 0))
      {
        /* Check if request to get ST.gif */
        if (strncmp((char const *)buf, "GET /STM32F7xx_files/ST.gif", 27) == 0)
        {
          fs_open(&file, "/STM32F7xx_files/ST.gif");
          netconn_write(conn, (const unsigned char* )(file.data), (size_t )file.len, NETCONN_NOCOPY);
          fs_close(&file);
        }
        /* Check if request to get stm32.jpeg */
        else if (strncmp((char const *)buf, "GET /STM32F7xx_files/stm32.jpg", 30) == 0)
        {
          fs_open(&file, "/STM32F7xx_files/stm32.jpg");
          netconn_write(conn, (const unsigned char* )(file.data), (size_t )file.len, NETCONN_NOCOPY);
          fs_close(&file);
        }
        else if (strncmp((char const *)buf, "GET /STM32F7xx_files/logo.jpg", 29) == 0)
        {
          /* Check if request to get ST logo.jpg */
          fs_open(&file, "/STM32F7xx_files/logo.jpg");
          netconn_write(conn, (const unsigned char* )(file.data), (size_t )file.len, NETCONN_NOCOPY);
          fs_close(&file);
        }
        else if (strncmp(buf, "GET /STM32F7xxTASKS.html", 24) == 0)
        {
          ServeDynamicWebPage(conn);
        }
        else if ((strncmp(buf, "GET /STM32F7xx.html", 19) == 0) || (strncmp(buf, "GET / ", 6) == 0))
        {
          /* Load STM32F7xx page */
          fs_open(&file, "/STM32F7xx.html");
          netconn_write(conn, (const unsigned char* )(file.data), (size_t )file.len, NETCONN_NOCOPY);
          fs_close(&file);
        }
        else
        {
          /* Load Error page */
          fs_open(&file, "/404.html");
          netconn_write(conn, (const unsigned char* )(file.data), (size_t )file.len, NETCONN_NOCOPY);
          fs_close(&file);
        }
      }
    }
  }
  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);

  /* Delete the buffer (netconn_recv gives us ownership,
   so we have to make sure to deallocate the buffer) */
  netbuf_delete(inbuf);
}

/**
 * @brief  http server thread
 * @param arg: pointer on argument(not used here)
 * @retval None
 */
static void HTTPServerThread(void *arg)
{
  struct netconn *conn, *newconn;
  err_t err, accept_err;

  /* Create a new TCP connection handle */
  conn = netconn_new(NETCONN_TCP);
  if (conn == NULL)
    JSB_ErrorHandler();

  /* Bind to interface */
  err = netconn_bind(conn, NULL, 80);
  if (err != ERR_OK)
    JSB_ErrorHandler();

  /* Listen for incoming connections */
  netconn_listen(conn);

  while (1)
  {
    /* Accept any incoming connection */
    accept_err = netconn_accept(conn, &newconn);
    if (accept_err == ERR_OK)
    {
      /* Serve connection */
      http_server_serve(newconn);

      /* Delete connection */
      netconn_delete(newconn);
    }
  }
}

void CreateHTTPServerThread_NetConn()
{
  if (!sys_thread_new("HTTPServer", HTTPServerThread, NULL, 1280, WEBSERVER_THREAD_PRIORITY))
  {
    printf("Failed to create HTTP server thread\r\n");
      JSB_ErrorHandler();
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

