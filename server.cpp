#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <curses.h>
#include <sys/select.h>
/* You will to add includes here */

#define DEBUG
#define VERSION "HELLO 1\n"
#define ERROR "ERROR TO\n"

int main(int argc, char *argv[])
{

  /* Do more magic */

  if (argc != 2)
  {
    printf("Wrong format IP:PORT\n");
    exit(0);
  }

  char delim[] = ":";
  char *Desthost = strtok(argv[1], delim);
  char *Destport = strtok(NULL, delim);

  if (Desthost == NULL || Destport == NULL)
  {
    printf("Wrong format.\n");
    exit(0);
  }
  int port = atoi(Destport);

  int sockfd, connfd, len;
  struct sockaddr_in cli;

  struct addrinfo sa, *si, *p;
  memset(&sa, 0, sizeof(sa));
  sa.ai_family = AF_UNSPEC;
  sa.ai_socktype = SOCK_STREAM;

  struct timeval tv;
  tv.tv_sec = 5;
  tv.tv_usec = 0;

  if (int rv = getaddrinfo(Desthost, Destport, &sa, &si) != 0)
  {
    fprintf(stderr, "%s\n", gai_strerror(rv));
    exit(0);
  }

  for (p = si; p != NULL; p = p->ai_next)
  {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      perror("Error: Couldnt connect.\n");
      continue;
    }

    if ((bind(sockfd, p->ai_addr, p->ai_addrlen)) != 0)
    {
      printf("Error: Couldnt bind!\n");
      close(sockfd);
      continue;
    }
    break;
  }

  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
  if (p == NULL)
  {
    printf("NULL\n");
    exit(0);
  }

  freeaddrinfo(si);

  if (listen(sockfd, 5) == -1)
  {
    printf("Error: Listen failed!\n");
    exit(0);
  }

  len = sizeof(cli);
  char buffer[128];
  char recvBuffer[256];
  fd_set currentSockets;
  fd_set readySockets;
  FD_ZERO(&currentSockets);
  FD_ZERO(&readySockets);
  FD_SET(sockfd, &currentSockets);
  int fdMax = sockfd;
  int nfds = 0;

  while (true)
  {
    readySockets = currentSockets;
    if (fdMax < sockfd)
    {
      fdMax = sockfd;
    }
    nfds = select(fdMax + 1, &readySockets, NULL, NULL, NULL);
    if ((connfd = accept(sockfd, (struct sockaddr *)&cli, (socklen_t *)&len)) == -1)
    {
      printf("Couldnt accept anything, trying again!\n");
      continue;
    }
    else
    {
      char sendbuf[128] = VERSION;
      send(connfd, sendbuf, strlen(sendbuf), 0);
      printf("Server protocol: %s", sendbuf);
    }

    memset(buffer, 0, sizeof(buffer));
    memset(recvBuffer, 0, sizeof(recvBuffer));
    if (recv(connfd, recvBuffer, sizeof(recvBuffer), 0) == -1)
    {
      close(connfd);
      continue;
    }
    else if (strstr(recvBuffer, "NICK") != nullptr)
    {
      printf("Name is allowed!\n");
    }
  }

#ifdef DEBUG
  printf("Host %s, and port %d.\n", Desthost, port);
#endif

  close(sockfd);
  return 0;
}
