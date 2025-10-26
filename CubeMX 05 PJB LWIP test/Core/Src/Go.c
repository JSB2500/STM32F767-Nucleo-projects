#include "stdio.h"
#include "string.h"
#include "FreeRTOS.h"
#include "sockets.h"
#include "lwip/ip_addr.h"

static void Listen()
{
  const int LocalPort = 5318;

  int fdListeningSocket, fdConnection;
  struct sockaddr_in LocalAddress, RemoteAddress; // JSB: "_in" suffix means "internet".
  socklen_t sizeof_RemoteAddress;
  char *RemoteIP;

  if ((fdListeningSocket = socket(PF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("General server: Failed to create listening socket.\r\n");
    Error_Handler();
  }

  /* Bind with address and port */
  memset(&LocalAddress, 0, sizeof(LocalAddress)); // JSB: Ensures that the unused parts of the structure are zeroed.
  LocalAddress.sin_family = AF_INET;
  LocalAddress.sin_addr.s_addr = htonl(INADDR_ANY); // Any interface.
  LocalAddress.sin_port = htons(LocalPort);
  if (bind(fdListeningSocket, (struct sockaddr *)&LocalAddress, sizeof (LocalAddress)) < 0)
  {
    printf("General server: Failed to bind.\r\n");
    Error_Handler();
  }

  /* Listen for incoming connections */
  listen(fdListeningSocket, 5);

  sizeof_RemoteAddress = sizeof(RemoteAddress);

  while (1)
  {
    fdConnection = accept(fdListeningSocket, (struct sockaddr * )&RemoteAddress, &sizeof_RemoteAddress);
    RemoteIP = inet_ntoa(RemoteAddress.sin_addr);
    printf("Connection accepted from %s\r\n", RemoteIP);
    // CreateGeneralServerConnectionThread(fdConnection);
  }
}

void Go()
{
  Listen();
}
