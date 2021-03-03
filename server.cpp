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
#include <vector>
/* You will to add includes here */

struct client
{
  int sockID;
  struct sockaddr_in addr;
  char name[12];
};
std::vector<client> clients;

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
  char buffer[273];
  char recvBuffer[260];
  char messageBuffer[256];
  char command[5];
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
    for (size_t i = 0; i < clients.size(); i++)
    {
      if (fdMax < clients.at(i).sockID)
      {
        fdMax = clients.at(i).sockID;
      }
    }
    if (fdMax < sockfd)
    {
      fdMax = sockfd;
    }
    nfds = select(fdMax + 1, &readySockets, NULL, NULL, NULL);
    if (nfds == -1)
    {
      printf("Something went wrong with the select\n");
      break;
    }

    if (FD_ISSET(sockfd, &readySockets))
    {
      if ((connfd = accept(sockfd, (struct sockaddr *)&cli, (socklen_t *)&len)) == -1)
      {
        continue;
      }
      else
      {
        struct client newClient;
        newClient.sockID = connfd;
        newClient.addr = cli;
        FD_SET(newClient.sockID, &currentSockets);
        clients.push_back(newClient);
        char buf[sizeof(VERSION)] = VERSION;
        send(connfd, buf, strlen(buf), 0);
        printf("Server protocol: %s\n", buf);
      }
      FD_CLR(sockfd, &readySockets);
    }

    for (size_t i = 0; i < clients.size(); i++)
    {
      if (FD_ISSET(clients.at(i).sockID, &readySockets))
      {
        memset(recvBuffer, 0, sizeof(recvBuffer));
        if (recv(clients.at(i).sockID, recvBuffer, sizeof(recvBuffer), 0) == -1)
        {
          continue;
        }
        else if (strstr(recvBuffer, "MSG ") != nullptr)
        {
          memset(buffer, 0, sizeof(buffer));
          memset(command, 0, sizeof(command));
          memset(messageBuffer, 0, sizeof(messageBuffer));
          sscanf(recvBuffer, "%s %[^\n]", command, messageBuffer);
          sprintf(buffer, "%s %s %s", command, clients.at(i).name, messageBuffer);
          for (size_t j = 0; j < clients.size(); j++)
          {
            if (j != i)
            {
              send(clients.at(j).sockID, buffer, strlen(buffer), 0);
            }
          }
        }
        else if (strstr(recvBuffer, "NICK ") != nullptr)
        {
          sscanf(recvBuffer, "%s %s", command, clients.at(i).name);
          printf("Name is allowed!\n");
        }
        FD_CLR(clients.at(i).sockID, &readySockets);
      }
    }
  }

#ifdef DEBUG
  printf("Host %s, and port %d.\n", Desthost, port);
#endif

  close(sockfd);
  return 0;
}
