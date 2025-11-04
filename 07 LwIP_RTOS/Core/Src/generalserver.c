#include "string.h"
#include "JSB_STM32F767_Nucleo144.h"
#include "JSB_General.h"
#include "main.h"
#include "generalserver.h"
//
#include "cmsis_os.h"
//
#include "lwip/sys.h"
#include "lwip/sockets.h"

static void GeneralServerConnectionThread(void *arg)
{
  const int ReadBufferLength = 1024;
  //
  int fdConnection;
  int ReadReturnValue;
  unsigned char ReadBuffer[ReadBufferLength];

  fdConnection = (int)arg;

  while (1)
  {
    ReadReturnValue = read(fdConnection, ReadBuffer, ReadBufferLength - 1);
    if (ReadReturnValue <= 0)
      break;

    ReadBuffer[Min_int32(ReadReturnValue, ReadBufferLength - 1)] = 0; // NULL terminator.

    printf((char *)ReadBuffer);
  }

  close(fdConnection);

  vTaskDelete(NULL);
}

static void CreateGeneralServerConnectionThread(int fdConnection)
{
  if (!sys_thread_new("GeneralConnection", GeneralServerConnectionThread, NULL, 1024, osPriorityAboveNormal))
  {
    printf("General server: Failed to create connection thread\r\n");
    JSB_ErrorHandler();
  }
}

static void GeneralServerListenerThread(void *arg)
{
  const int LocalPort = 5318;

  int fdListeningSocket, fdConnection;
  struct sockaddr_in LocalAddress, RemoteAddress; // JSB: "_in" suffix means "internet".
  socklen_t sizeof_RemoteAddress;
  char *RemoteIP;

  if ((fdListeningSocket = socket(PF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("General server: Failed to create listening socket.\r\n");
    JSB_ErrorHandler();
  }

  /* Bind with address and port */
  memset(&LocalAddress, 0, sizeof(LocalAddress)); // JSB: Ensures that the unused parts of the structure are zeroed.
  LocalAddress.sin_family = AF_INET;
  LocalAddress.sin_addr.s_addr = htonl(INADDR_ANY); // Any interface.
  LocalAddress.sin_port = htons(LocalPort);
  if (bind(fdListeningSocket, (struct sockaddr *)&LocalAddress, sizeof (LocalAddress)) < 0)
  {
    printf("General server: Failed to bind.\r\n");
    JSB_ErrorHandler();
  }

  /* Listen for incoming connections */
  listen(fdListeningSocket, 5);

  sizeof_RemoteAddress = sizeof(RemoteAddress);

  while (1)
  {
    fdConnection = accept(fdListeningSocket, (struct sockaddr * )&RemoteAddress, &sizeof_RemoteAddress);
    RemoteIP = inet_ntoa(RemoteAddress.sin_addr);
    printf("Connection accepted from %s\r\n", RemoteIP);
    CreateGeneralServerConnectionThread(fdConnection);
  }
}

void CreateGeneralServerListenerThread()
{
  if (!sys_thread_new("GeneralServerListener", GeneralServerListenerThread, NULL, 1024, osPriorityAboveNormal))
  {
    printf("General server: Failed to create listening thread.\r\n");
    JSB_ErrorHandler();
  }
}
