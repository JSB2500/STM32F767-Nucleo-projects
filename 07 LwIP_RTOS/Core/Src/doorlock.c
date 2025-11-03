#include <doorlock.h>
#include "string.h"
#include "JSB_STM32F767ZINucleo144.h"
#include "JSB_General.h"
#include "main.h"
#include "cmsis_os.h"
//
#include "lwip/sys.h"
#include "lwip/sockets.h"

#define ServerIP "137.74.43.80" /* PJB OVH */
//#define ServerIP "192.168.1.13" /* JSB Local test server */
//#define ServerIP "151.80.60.118" /* JSB Remote test server (OVH) */
#define ServerDoorClientPort 18753

#if JSBGeneral_Sockets
#include <sys/time.h>
#include "lwip/sys.h"
#include "lwip/sockets.h"
#endif

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

static void DoorLockClientThread(void *arg)
{
  int fdClientSocket;
  int Locked = 0;
  char ReceivedChar, SendChar;
  char Message[128], S[128];

  while (1)
  {
    while (1)
    {
      printf("Door client: Attempting to connect to server on IP: %s.\r\n", ServerIP);
      if ((fdClientSocket = JSB_Sockets_Connect(ServerIP, ServerDoorClientPort, SOCK_STREAM)) >= 0)
      {
        printf("Door client: Connected.\r\n");
        break;
      }
      osDelay(1000);
    }

    strcpy(Message, "GET /arduino HTTP/1.1\r\n\r\n");
    send(fdClientSocket, &Message, strlen(Message), 0);

    while (1)
    {
      errno = 0;
      if (recv(fdClientSocket, &ReceivedChar, 1, 0) <= 0)
      {
        printf("Door client: Connection lost. Socket error: %d.\r\n", errno);
        break;
      }

      printf("Door client: Received '%c'.\r\n", ReceivedChar);

      if (ReceivedChar == 'p')
      {
        printf("Command: Ping.\r\n");
        SendChar = !Locked ? 'o' : 'c';
        printf("Door client: Sending '%c'.\r\n", SendChar);
        if (send(fdClientSocket, (void *)&SendChar, 1, 0) < 0)
          break;
      }
      else if (ReceivedChar == 'o')
      {
        printf("Command: Open.\r\n");
        Locked = !Locked;
        if (Locked)
          strcpy(S, "Locked");
        else
          strcpy(S, "Unlocked");
      }
    }

    close(fdClientSocket);
  }
}

void CreateDoorLockClientThread()
{
  if (!sys_thread_new("DoorClient", DoorLockClientThread, NULL, 1024, osPriorityAboveNormal))
  {
    printf("Failed to create door client thread.\r\n");
    JSB_ErrorHandler();
  }
}

