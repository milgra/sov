#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

static const char*        socket_path          = "/home/mysocket";
static const unsigned int nIncomingConnections = 5;

int main()
{
  // create server side
  int                s  = 0;
  int                s2 = 0;
  struct sockaddr_un local, remote;
  int                len = 0;

  s = socket(AF_UNIX, SOCK_STREAM, 0);
  if (-1 == s)
  {
    printf("Error on socket() call \n");
    return 1;
  }

  local.sun_family = AF_UNIX;
  strcpy(local.sun_path, socket_path);
  unlink(local.sun_path);
  len = strlen(local.sun_path) + sizeof(local.sun_family);
  if (bind(s, (struct sockaddr*)&local, len) != 0)
  {
    printf("Error on binding socket \n");
    return 1;
  }

  if (listen(s, nIncomingConnections) != 0)
  {
    printf("Error on listen call \n");
  }

  bool bWaiting = true;
  while (bWaiting)
  {
    unsigned int sock_len = 0;
    printf("Waiting for connection.... \n");
    if ((s2 = accept(s, (struct sockaddr*)&remote, &sock_len)) == -1)
    {
      printf("Error on accept() call \n");
      return 1;
    }

    printf("Server connected \n");

    int  data_recv = 0;
    char recv_buf[100];
    char send_buf[200];
    do {
      memset(recv_buf, 0, 100 * sizeof(char));
      memset(send_buf, 0, 200 * sizeof(char));
      data_recv = recv(s2, recv_buf, 100, 0);
      if (data_recv > 0)
      {
        printf("Data received: %d : %s \n", data_recv, recv_buf);
        strcpy(send_buf, "Got message: ");
        strcat(send_buf, recv_buf);

        if (strstr(recv_buf, "quit") != 0)
        {
          printf("Exit command received -> quitting \n");
          bWaiting = false;
          break;
        }

        if (send(s2, send_buf, strlen(send_buf) * sizeof(char), 0) == -1)
        {
          printf("Error on send() call \n");
        }
      }
      else
      {
        printf("Error on recv() call \n");
      }
    } while (data_recv > 0);

    close(s2);
  }

  return 0;
}
