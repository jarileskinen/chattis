#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MYPORT "3490"
#define BACKLOG 10
#define BUFSIZE 10000

typedef struct sockaddr_storage sockaddr_storage;
typedef struct sockaddr sockaddr;
typedef struct addrinfo addrinfo;

void *get_in_addr(sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char const *argv[]) {
  sockaddr_storage remoteaddr;
  socklen_t addrlen;
  addrinfo hints, *p;
  int status, newfd, listener, fdmax, nbytes;
  char buf[BUFSIZE];
  fd_set master;
  fd_set readfs;
  char remoteIP[INET6_ADDRSTRLEN];

  int i, j;

  FD_ZERO(&master);
  FD_ZERO(&readfs);

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((status = getaddrinfo(NULL, MYPORT, &hints, &p)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return 2;
  }

  if ((listener = socket(p->ai_family, p->ai_socktype,
                       p->ai_protocol)) == -1) {
      perror("Socket error");
      return 2;
  }

  if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
    perror("Bind error");
    return 2;
  }

  /*freeaddrinfo(ai);*/

  if (listen(listener, BACKLOG) == -1) {
    perror("Listen error");
    return 2;
  }
  printf("Now listening on port %s\n", MYPORT);

  FD_SET(listener, &master);

  fdmax = listener;

  while (1) {
    readfs = master;
    if (select(fdmax+1, &readfs, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(4);
    }
    
    for (i = 0; i <= fdmax; ++i) {
      if (FD_ISSET(i, &readfs)) {
        if (i == listener) {
          addrlen = sizeof remoteaddr;
          newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
          printf("%s\n", "Connected!");

          if (newfd == -1) {
            perror("Accept");
          } else {
            FD_SET(newfd, &master);
            if (newfd > fdmax) {
              fdmax = newfd;
            }
            printf("Selectserver: new connection from %s on "
                "socket %d\n", inet_ntop(remoteaddr.ss_family,
                  get_in_addr((struct sockaddr*)&remoteaddr),
                  remoteIP, INET6_ADDRSTRLEN),
                newfd);
          }
        } else {
          if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
            if (nbytes == 0) {
              printf("selectserver: socket %d hung up\n", i);
            } else {
              perror("recv");
            }
            close(i);
            FD_CLR(i, &master);
          } else {
            for (j = 0; j <= fdmax; j++) {
              if (FD_ISSET(j, &master)) {
                if (j != listener && j != i) {
                  if (send(j, buf, nbytes, 0) == -1) {
                    perror("send");
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  return 0;
}
