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
#include <regex.h>
#include <string>

/* You will to add includes here */

struct client
{
  int sockID;
  struct sockaddr_in addr;
  char name[12];
};
std::vector<client> clients;

void INThandler(int sig)
{
  for (size_t i; i < clients.size(); i++)
  {
    send(clients[i].sockID, "Server is closing!\n", strlen("Server is closing!\n"), 0);
  }
  exit(0);
}

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
  sa.ai_flags = AI_PASSIVE;

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

  if (p == NULL)
  {
    printf("NULL\n");
    exit(0);
  }

  freeaddrinfo(si);
  signal(SIGINT, INThandler);

  if (listen(sockfd, 1) == -1)
  {
    printf("Error: Listen failed!\n");
    exit(0);
  }

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

  len = sizeof(cli);
  char buffer[274];
  char recvBuffer[260];
  char messageBuffer[256];
  char command[5];
  char nameLength[15];
  fd_set currentSockets;
  fd_set readySockets;
  FD_ZERO(&currentSockets);
  FD_ZERO(&readySockets);
  FD_SET(sockfd, &currentSockets);
  int fdMax = sockfd;
  int nfds = 0;
  int recieve = 0;
  bool nameExists = false;
  //int currentClient = -1;

  while (true)
  {
    readySockets = currentSockets;

    nfds = select(fdMax + 1, &readySockets, NULL, NULL, NULL);
    if (nfds == -1)
    {
      printf("Something went wrong with the select\n");
      break;
    }

    for (int i = sockfd; i < fdMax + 1; i++)
    {

      if (FD_ISSET(i, &readySockets))
      {
        if (i == sockfd)
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
            if (newClient.sockID > fdMax)
            {
              fdMax = newClient.sockID;
            }
          }
        }
        else
        {
          std::string temp = {};
          memset(recvBuffer, 0, sizeof(recvBuffer));
          recieve = recv(i, recvBuffer, sizeof(recvBuffer), 0);
          if (recieve <= 0)
          {
            close(i);
            for (size_t j = 0; j < clients.size(); j++)
            {
              if (i == clients.at(j).sockID)
              {
                clients.erase(clients.begin() + j);
                FD_CLR(i, &currentSockets);
                break;
              }
            }
            continue;
          }
          else
          {
            temp = recvBuffer;
            while (strlen(recvBuffer) > 0)
            {
              if (strstr(recvBuffer, "MSG ") != nullptr)
              {
                memset(buffer, 0, sizeof(buffer));
                memset(command, 0, sizeof(command));
                memset(messageBuffer, 0, sizeof(messageBuffer));
                sscanf(recvBuffer, "%s %[^\n]", command, messageBuffer);
                for (size_t j = 0; j < clients.size(); j++)
                {
                  if (i == clients.at(j).sockID)
                  {
                    sprintf(buffer, "%s %s %s\n", command, clients.at(j).name, messageBuffer);
                    break;
                  }
                }
                for (size_t j = 0; j < clients.size(); j++)
                {
                  send(clients.at(j).sockID, buffer, strlen(buffer), 0);
                }
                temp.erase(0, strlen(command) + strlen(messageBuffer) + 2);
                strcpy(recvBuffer, temp.c_str());
              }
              else if (strstr(recvBuffer, "NICK ") != nullptr)
              {
                nameExists = false;
                //currentClient = -1;
                for (size_t j = 0; j < clients.size(); j++)
                {
                  if (i == clients.at(j).sockID)
                  {
                    memset(nameLength, 0, sizeof(nameLength));
                    //currentClient = j;
                    sscanf(recvBuffer, "%s %[^\n]", command, nameLength);
                    if (strlen(nameLength) > 12)
                    {
                      nameExists = true;
                      send(i, "ERROR, Name is to long, Max 12 characters is allowed!\n", strlen("ERROR, Name is to long, Max 12 characters is allowed!\n"), 0);
                    }
                    else
                    {
                      sscanf(recvBuffer, "%s %s", command, clients.at(j).name);
                    }
                    for (size_t g = 0; g < clients.size() && !nameExists; g++)
                    {
                      if (g != j && strcmp(clients.at(g).name, clients.at(j).name) == 0 && strlen(clients.at(g).name) == strlen(clients.at(j).name))
                      {
                        nameExists = true;
                        send(i, "ERROR The name is already in use!\n", strlen("ERROR The name is already in use!\n"), 0);
                      }
                    }
                    if (nameExists == false)
                    {

                      reti = regexec(&regularexpression, clients.at(j).name, matches, &items, 0);
                      if (reti)
                      {
                        printf("Nick is not accepted.\n");
                        send(i, "ERROR, Characters not OK!\n", strlen("ERROR, Characters not OK!\n"), 0);
                      }
                      else
                      {
                        printf("Name is allowed!\n");
                        send(i, "OK\n", strlen("OK\n"), 0);
                        break;
                      }
                    }
                  }
                }
                temp.erase(0, strlen(command) + strlen(nameLength) + 2);
                strcpy(recvBuffer, temp.c_str());
              }
              else
              {
                send(i, "ERROR Wrong format on the message\n", strlen("ERORR Wrong format on the message\n"), 0);
              }
            }
          }
        }

        FD_CLR(i, &readySockets);
      }
    }
  }

#ifdef DEBUG
  printf("Host %s, and port %d.\n", Desthost, port);
#endif
  regfree(&regularexpression);
  close(sockfd);
  return 0;
}
