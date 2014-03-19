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

int setup_socket(addrinfo *hints, int *listener);
int accept_connection(int listener);
void close_connection(int fd, int nbytes);

int main(int argc, char const *argv[]) {
  addrinfo hints;
  int listener, fdmax, nbytes;
  char buf[BUFSIZE];
  fd_set master;
  fd_set readfs;

  FD_ZERO(&master);
  FD_ZERO(&readfs);

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  setup_socket(&hints, &listener); 

  /*freeaddrinfo(ai);*/

  if (listen(listener, BACKLOG) == -1) {
    perror("Listen error");
    return 2;
  }
  printf("Now listening on port %s\n", MYPORT);

  FD_SET(listener, &master);

  fdmax = listener;

  while (1) {
    int i;

    readfs = master;
    if (select(fdmax+1, &readfs, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(4);
    }

    for (i = 0; i <= fdmax; ++i) {
      if (!FD_ISSET(i, &readfs)) continue;

      if (i == listener) {
        int newfd;
        if (!(newfd = accept_connection(listener))) continue;

        FD_SET(newfd, &master);
        fdmax = newfd > fdmax ? newfd : fdmax;
      } else {
        if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
          close_connection(i, nbytes);
          FD_CLR(i, &master);
        } else {
          int j;
          for (j = 0; j <= fdmax; j++) {
            if (!FD_ISSET(j, &master)) continue;
            if (j == listener) continue;
            if (j == i) continue;
            
            if (send(j, buf, nbytes, 0) == -1) {
              perror("send");
            }
          }
        }
      }
    }
  }

  return 0;
}

void init() {

}

int setup_socket(addrinfo *hints, int *listener) {
  addrinfo *p;
  int status;

  if ((status = getaddrinfo(NULL, MYPORT, hints, &p)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    exit(2);
  }

  if ((*listener = socket(p->ai_family, p->ai_socktype,
                       p->ai_protocol)) == -1) {
    perror("Socket error");
    exit(2);
  }

  if (bind(*listener, p->ai_addr, p->ai_addrlen) == -1) {
    perror("Bind error");
    exit(2);
  }

  return 1;
}

int accept_connection(int listener) {
  int newfd;
  socklen_t addrlen;
  sockaddr_storage remoteaddr;
  char remoteIP[INET6_ADDRSTRLEN];
  
  addrlen = sizeof(remoteaddr);
  newfd = accept(listener,(sockaddr*)&remoteaddr, &addrlen);
  if (newfd == -1) {
    perror("Accept");
    return 0;
  }
 
  printf("New connection from %s on "
            "socket %d\n", inet_ntop(remoteaddr.ss_family,
            get_in_addr((sockaddr*)&remoteaddr),
            remoteIP, INET6_ADDRSTRLEN),
            newfd);
  return newfd;
}

void close_connection(int fd, int nbytes) {
  if (nbytes == 0) {
    printf("Closed socket %d", fd);
  } else {
    perror("recv");
  }
  close(fd);
}
