#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUFSIZE 10000

typedef struct addrinfo addrinfo;

int main(int argc, const char *argv[])
{
  addrinfo hints, *res;
  int status, sockfd, maxfd;
  char buf[BUFSIZE];
  fd_set masterrfs, masterwfs, readfs, writefs;

  if (argc != 3) {
    fprintf(stderr, "usage: chat hostname port\n");
    return 1;
  }

  FD_ZERO(&masterrfs);
  FD_ZERO(&masterwfs);
  FD_ZERO(&readfs);
  FD_ZERO(&writefs);

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((status = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return 2;
  }

  if ((sockfd = socket(res->ai_family, res->ai_socktype,
                       res->ai_protocol)) == -1) {
      perror("Socket error");
      return 2;
  }

  if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
    perror("Unable to connect");
    return 2;
  }

  /*while (fgets(buf, BUFSIZE, stdin) != NULL) {
    send(sockfd, buf, BUFSIZE, 0);
    recv(sockfd, buf, BUFSIZE, 0);
    printf("%s\n", buf);
  }*/

  FD_SET(STDIN_FILENO, &masterrfs);
  FD_SET(sockfd, &masterrfs);

  maxfd = sockfd;

  while (1) {
    readfs = masterrfs;
    writefs = masterwfs;

    if (select(maxfd+1, &readfs, NULL, NULL, 0) == -1) {
      perror("select");
      return 1;
    }

    if (FD_ISSET(STDIN_FILENO, &readfs)) {
      fgets(buf, BUFSIZE, stdin);
      send(sockfd, buf, BUFSIZE, 0);
    }
    if (FD_ISSET(sockfd, &readfs)) {
      recv(sockfd, buf, BUFSIZE, 0);
      printf("%s\n", buf);
    }
  }

  freeaddrinfo(res);
  close(sockfd);

  return 0;
}
