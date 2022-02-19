#ifndef remote_h
#define remote_h

#include "zc_channel.c"

void remote_listen(ch_t* channel, int port);
void remote_close();

#endif

#if __INCLUDE_LEVEL__ == 0

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXLINE 10

struct _remote_t
{
  int port;
  int alive;
} rem;

void* remote_listen_ins(void* p)
{
  ch_t* channel = (ch_t*)p;

  char               buffer[MAXLINE];
  struct sockaddr_in servaddr, cliaddr;

  // Creating socket file descriptor
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) printf("socket creation failed");

  // zero out addresses
  memset(&servaddr, 0, sizeof(servaddr));
  memset(&cliaddr, 0, sizeof(cliaddr));

  // Filling server information
  servaddr.sin_family      = AF_INET; // IPv4
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port        = htons(rem.port);

  // Bind the socket with the server address

  int res = bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr));
  if (res < 0) printf("socket bind failed");

  socklen_t len = sizeof(cliaddr); // len is value/resuslt

  while (rem.alive)
  {
    // blocking listen
    int num = recvfrom(sockfd,
                       (char*)buffer,
                       MAXLINE,
                       MSG_WAITALL,
                       (struct sockaddr*)&cliaddr,
                       &len);

    buffer[num] = '\0';

    ch_send(channel, buffer);
  }

  close(sockfd);
  return NULL;
}

void remote_listen(ch_t* channel, int port)
{
  pthread_t threadId;

  if (rem.alive == 0)
  {
    rem.alive = 1;
    rem.port  = port;

    int err = pthread_create(&threadId, NULL, &remote_listen_ins, channel);

    if (err) printf("Thread creation failed : %s", strerror(err));
  }
}

void remote_close()
{
  rem.alive = 0;
}

#endif
