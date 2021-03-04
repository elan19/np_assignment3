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
#include <sys/select.h>
#include <iostream>
#include <signal.h>
/* You will to add includes here */

#define DEBUG
#define VERSION "HELLO 1\n"
#define ERROR "ERROR TO\n"
#define SERVERQUIT "Server is closing!\n"

void INThandler(int sig)
{
  printf("\n");
  exit(0);
}

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
  char expression[] = "^[A-Za-z0-9_]+$";
  regex_t regularexpression;
  int reti;

  reti = regcomp(&regularexpression, expression, REG_EXTENDED);
  if (reti)
  {
    fprintf(stderr, "Could not compile regex.\n");
    exit(1);
  }

  int matches = 0;
  regmatch_t items;

  if (Desthost == NULL || Destport == NULL || DestName == NULL)
  {
    printf("Wrong format.\n");
    exit(0);
  }
  if (strlen(DestName) > 12)
  {
    printf("Error, name to long!\n");
    exit(1);
  }

  reti = regexec(&regularexpression, argv[2], matches, &items, 0);
  if (reti)
  {
    printf("Nick is not accepted.\n");
    exit(0);
  }

  regfree(&regularexpression);
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
  signal(SIGINT, INThandler);

  char recvBuf[273];
  char sendBuf[260];
  int bytes;
  fd_set currentSockets;
  fd_set readySockets;
  FD_ZERO(&currentSockets);
  FD_ZERO(&readySockets);
  FD_SET(sockfd, &currentSockets);
  FD_SET(STDIN_FILENO, &currentSockets);
  int fdMax = sockfd;
  int nfds = 0;
  char messageBuf[256];
  char command[8];
  char buffer[256];
  char nameBuffer[12];

  while (1)
  {
    readySockets = currentSockets;
    if (fdMax < sockfd)
    {
      fdMax = sockfd;
    }
    nfds = select(fdMax + 1, &readySockets, NULL, NULL, NULL);
    if (nfds == -1)
    {
      printf("ERROR, something went wrong!\n");
      break;
    }
    if (FD_ISSET(STDIN_FILENO, &readySockets))
    {
      memset(sendBuf, 0, sizeof(sendBuf));
      memset(messageBuf, 0, sizeof(messageBuf));
      std::cin.getline(messageBuf, sizeof(messageBuf));
      std::cin.clear();
      if (strlen(messageBuf) > 256)
      {
        printf("TO BIG MESSAGE\n");
        FD_CLR(STDIN_FILENO, &readySockets);
        memset(messageBuf, 0, sizeof(messageBuf));
        break;
      }
      else
      {
        //sscanf(recvBuf, "%s %s", command, buffer);
        sprintf(sendBuf, "MSG %s", messageBuf);
        send(sockfd, sendBuf, sizeof(sendBuf), 0);
        FD_CLR(STDIN_FILENO, &readySockets);
      }
    }
    if (FD_ISSET(sockfd, &readySockets))
    {
      memset(recvBuf, 0, sizeof(recvBuf));
      if ((bytes = recv(sockfd, recvBuf, sizeof(recvBuf), 0)) == -1)
      {
        continue;
      }
      else
      {
        memset(command, 0, sizeof(command));
        sscanf(recvBuf, "%s", command);
      }
      if (strstr(command, "MSG"))
      {
        memset(buffer, 0, sizeof(buffer));
        memset(command, 0, sizeof(command));
        memset(nameBuffer, 0, sizeof(nameBuffer));
        sscanf(recvBuf, "%s %s %[^\n]", command, nameBuffer, buffer);
        if (strstr(nameBuffer, DestName) == nullptr)
        {
          printf("%s: %s\n", nameBuffer, buffer);
        }
      }
      else if (strstr(recvBuf, VERSION) != nullptr)
      {
        memset(sendBuf, 0, sizeof(sendBuf));
        printf("Server protocol: %s\n", recvBuf);
        sprintf(sendBuf, "NICK %s", DestName);
        send(sockfd, sendBuf, strlen(sendBuf), 0);
      }
      else if (strstr(recvBuf, "OK") != nullptr)
      {
        printf("Name accepted!\n");
      }
      else if (strstr(recvBuf, "ERR") != nullptr)
      {
        printf("Name is not accepted!\n");
      }
      else if(strstr(recvBuf, "Server is closing!\n") != nullptr)
      {
        printf("%s", recvBuf);
        exit(0);
      }
      FD_CLR(sockfd, &readySockets);
    }
  }

#ifdef DEBUG
  printf("Host %s, and port %d.\n", Desthost, port);
#endif

  close(sockfd);
  return 0;
}
