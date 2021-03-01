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
#include <regex.h>
/* You will to add includes here */

#define DEBUG
#define VERSION "HELLO 1\n"
#define ERROR "ERROR TO\n"

int main(int argc, char *argv[])
{

  /* Do magic */

  if (argc != 3)
  {
    printf("Wrong format IP:PORT:NAME\n");
    exit(0);
  }

  char delim[] = ":";
  char *Desthost = strtok(argv[1], delim);
  char *Destport = strtok(NULL, delim);
  char *DestName = strtok(argv[2], delim);

  if (Desthost == NULL || Destport == NULL || DestName == NULL)
  {
    printf("Wrong format.\n");
    exit(0);
  }
  if(strlen(DestName) > 12)
  {
    printf("Error, name to long!\n");
    exit(1);
  }
  printf("%s\n", DestName);
  int port = atoi(Destport);

  addrinfo sa, *si, *p;
  memset(&sa, 0, sizeof(sa));
  sa.ai_family = AF_INET;
  sa.ai_socktype = SOCK_STREAM;

  struct timeval tv;
  tv.tv_sec = 5;
  tv.tv_usec = 0;

  if (int rv = getaddrinfo(Desthost, Destport, &sa, &si) != 0)
  {
    fprintf(stderr, "%s\n", gai_strerror(rv));
    exit(0);
  }

  int sockfd;

  for (p = si; p != NULL; p = p->ai_next)
  {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      perror("Error: Couldnt connect.\n");
      continue;
    }

    if ((connect(sockfd, p->ai_addr, p->ai_addrlen) == -1))
    {
      close(sockfd);
      printf("Error: Couldnt connect.\n");
      continue;
    }
    break;
  }

  if (p == NULL)
  {
    printf("NULL\n");
    exit(0);
  }
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
  freeaddrinfo(si);

  char recvBuf[256];
  char sendBuf[256];
  int bytes;

  while (1)
  {
    if ((bytes = recv(sockfd, recvBuf, sizeof(recvBuf), 0)) == -1)
    {
      continue;
    }
    else if(strstr(recvBuf, VERSION) != nullptr)
    {
      printf("Server protocol: %s\n", recvBuf);
      sprintf(sendBuf, "NICK%s",DestName);
      send(sockfd, sendBuf, strlen(sendBuf), 0);
    }
  }

#ifdef DEBUG
  printf("Host %s, and port %d.\n", Desthost, port);
#endif

  close(sockfd);
  return 0;
}
