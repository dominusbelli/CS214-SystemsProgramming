#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "libnetfiles.h"

// Global Variables
int fdtracker[512] = {0};
int fdcount = 0;

void *msg_handler(void *vargp) {
  MsgData *first = (MsgData *)vargp;
  char buffer[MAXBUFFERSIZE];
  int errnopayload;

  // NetOpen: 1
  // NetRead: 2
  // NetWrite: 3
  // NetClose: 4
  switch (first->msg_type) {
    case NETOPEN:
      printf("NetOpen: Request from %s\n", first->ip_address);
      // wait for pathname
      printf("NetOpen: Waiting for path message...\n");
      int pathmsg;
      if ((pathmsg = recv(first->clientfd, buffer, MAXBUFFERSIZE, 0)) == -1) {
        perror("netopen path");
        exit(1);
      }
      buffer[pathmsg] = '\0';
      printf("NetOpen: Received path: %s\n", buffer);

      printf("NetOpen: Waiting for flags message...\n");
      int flagsrecv;
      int flagsmsg;
      if ((flagsmsg = recv(first->clientfd, &flagsrecv, sizeof(int), 0)) == -1) {
        perror("netopen flags");
      }
      int flags = ntohl(flagsrecv);
      printf("NetOpen: Received flags: %i\n", flags);

      // try actually open()ing the file and then sending the result FD back
      printf("NetOpen: Trying to open the file\n");
      int result = open(buffer, flags);
      printf("NetOpen: Sending netopen result: %i\n", result);
      int resultpayload = htonl(result);
      if (send(first->clientfd, &resultpayload, sizeof(int), 0) == -1) {
        perror("netopen send result");
      }
      // if there was an error getting the resulting FD
      if (result == -1) {
        // send serrno across the wire
        printf("NetOpen: Sending errno :%d\n", errno);
        errnopayload = htonl(errno);
        if (send(first->clientfd, &errnopayload, sizeof(errnopayload), 0) == -1) {
          perror("netopen send errno");
        }
      }
      // Valid FD for opening file. Add to tracker
      if (fdcount<512) {
        fdtracker[fdcount] = resultpayload;
      } else {
        printf("NetOpen: Too many files open. Unable to save FD.\n");
        fdcount++;
      }
      printf("NetOpen: Finished Operation.\n");
      close(first->clientfd);
      break;

    case NETREAD:
      printf("NetRead: Requested from %s\n", first->ip_address);

      // Waiting for file descriptor
      printf("NetRead: Waiting for File Descriptor...\n");
      int filedesrecv;
      int filedesmsg;
      if ((filedesmsg = recv(first->clientfd, &filedesrecv, sizeof(filedesrecv), 0)) == -1) {
        perror("netread filedesmsg");
      }
      int readfd = ntohl(filedesrecv);
      printf("NetRead: Received File Descriptor: %i\n", readfd);
      if(readfd < 0) {
        printf("%d\n", readfd);
      }

      // Waiting for nbyte size
      printf("NetRead: Waiting for nbyte size...\n");
      size_t nbyterecv;
      int nbytemsg;
      if ((nbytemsg = recv(first->clientfd, &nbyterecv, sizeof(nbyterecv), 0)) == -1) {
        perror("netread nbyte");
      }
      size_t nbyte = ntohl(nbyterecv);
      printf("NetRead: Received nbyte: %d\n", nbyte);

      sleep(1);
      // Time to actually read() the file
      printf("NetRead: Trying to read the file\n");
      bzero(buffer, MAXBUFFERSIZE);
      //printf("readfd: %d, nbyte: %d\n", readfd, nbyte);
      int readresult = read(readfd, buffer, nbyte);
      printf("NetRead: Buffer Result: %s\n", buffer);
      int readresultpayload = htonl(readresult);
      if (send(first->clientfd, &readresultpayload, sizeof(readresultpayload), 0) == -1) {
        perror("netread send result");
      }
      // if there was an error getting the resulting size
      if (readresult == -1) {
        // send serrno across the wire
        printf("NetRead: Sending errno :%d\n", errno);
        errnopayload = htonl(errno);
        if (send(first->clientfd, &errnopayload, sizeof(errnopayload), 0) == -1) {
          perror("netread send errno");
        }
      } else {
        // send the buffer that was read
        printf("NetRead: Sending buffer: %s\n", buffer);
        if (send(first->clientfd, buffer, readresult, 0) == -1) {
          perror("netread send buffer");
        }
      }
      printf("NetRead: Finished Operation.\n");
      close(first->clientfd);
      break;

    case NETWRITE:
      printf("NetWrite: Request from %s\n", first->ip_address);

      // Waiting for file descriptor
      printf("NetWrite: Waiting for File Descriptor...\n");
      int fdrecv;
      int fdmsg;
      if ((fdmsg = recv(first->clientfd, &fdrecv, sizeof(fdrecv), 0)) == -1) {
        perror("netwrite filedesmsg");
      }
      int writefd = ntohl(fdrecv);
      printf("NetWrite: Received File Descriptor: %i\n", readfd);
      if(writefd < 0) {
        printf("%d\n", writefd);
      }

      // Waiting for nbyte size
      printf("NetWrite: Waiting for nbyte size...\n");
      size_t writenbyterecv;
      int writenbytemsg;
      if ((writenbytemsg = recv(first->clientfd, &nbyterecv, sizeof(writenbyterecv), 0)) == -1) {
        perror("netwrite nbyte");
      }
      size_t writenbyte = ntohl(writenbyterecv);
      printf("Netwrite: Received nbyte: %d\n", writenbyte);

      // wait for buffer
      printf("NetWrite: Waiting for string to write in file...\n");
      int stringmsg;
      if ((stringmsg = recv(first->clientfd, buffer, MAXBUFFERSIZE, 0)) == -1) {
        perror("netwrite path");
        exit(1);
      }
      buffer[stringmsg] = '\0';
      printf("NetWrite: Received path: %s\n", buffer);

      sleep(1);
      // Time to actually read() the file
      printf("NetWrite: Trying to write to the file\n");
      //printf("readfd: %d, nbyte: %d\n", readfd, nbyte);
      int writeresult = write(writefd, buffer, writenbyte);
      printf("NetWrite: Write Result: %s\n", writeresult);
      int writeresultpayload = htonl(readresult);
      if (send(first->clientfd, &writeresultpayload, sizeof(writeresultpayload), 0) == -1) {
        perror("netwrite send result");
      }
      // if there was an error getting the resulting size
      if (writeresult == -1) {
        // send serrno across the wire
        printf("NetWrite: Sending errno :%d\n", errno);
        errnopayload = htonl(errno);
        if (send(first->clientfd, &errnopayload, sizeof(errnopayload), 0) == -1) {
          perror("netwrite send errno");
        }
      }
      printf("NetWrite: Finished Operation.\n");
      close(first->clientfd);
      break;
    case NETCLOSE:
      printf("NetClose requested from %s\n", first->ip_address);

      // Waiting for file descriptor
      printf("NetClose: Waiting for File Descriptor...\n");
      int closerecv;
      int closemsg;
      if ((closemsg = recv(first->clientfd, &closerecv, sizeof(closerecv), 0)) == -1) {
        perror("netclose closemsg");
      }
      int closefd = ntohl(filedesrecv);
      printf("NetClose: Received File Descriptor: %i\n", closefd);
      if(closefd < 0) {
        printf("%d\n", closefd);
      }

      sleep(1);

      int closed = close(closefd);
      int closedresultpayload = htonl(closed);
      if (send(first->clientfd, &closedresultpayload, sizeof(closedresultpayload), 0) == -1) {
        perror("netwrite send result");
      }
      // if there was an error getting the resulting size
      //error: send errno back
      if (closed == -1) {
        printf("NetClose: Sending errno :%d\n", errno);
        errnopayload = htonl(errno);
        if (send(first->clientfd, &errnopayload, sizeof(errnopayload), 0) == -1) {
          perror("netclose send errno");
        }
      }
      printf("NetClose: Finished Operation.\n");
      close(first->clientfd);
      break;
  }
}

int main(int argc, char const *argv[]) {
  int sockfd;
  int recfd;
  struct addrinfo hints;
  struct addrinfo *res;

  printf("Spinning up netfileserver!\n");

  // set up hints for getaddrinfo
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  getaddrinfo(NULL, PORT, &hints, &res);

  // set up the socket descriptor
  printf("Setting up socket descriptor...\n");
  sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

  // binds the socket descriptor to the ip
  printf("Binding socket descriptor to IP Address Info...\n");
  bind(sockfd, res->ai_addr, res->ai_addrlen);

  // sets the socket descriptor to listen for connections
  listen(sockfd, 10);

  char currentAddr[INET_ADDRSTRLEN];
  struct sockaddr_storage clientAddr;
  socklen_t addr_size;
  while (1) {
    // accept() waits for connections to come in to connect to
    printf("Waiting to accept new connections...\n");
    addr_size = sizeof clientAddr;
    recfd = accept(sockfd, (struct sockaddr *)&clientAddr, &addr_size);
    if (recfd == -1) {
      perror("accept");
    }

    // inet_ntop puts the client's ip into a string to work with
    inet_ntop(clientAddr.ss_family, (void *)((struct sockaddr *)res->ai_addr), currentAddr, sizeof currentAddr);
    printf("Client connected: %s\n", currentAddr);

    // Set up message to pass to msg_handler()
    MsgData *data = malloc(sizeof(MsgData));
    strcpy(data->ip_address, currentAddr);
    data->clientfd = recfd;

    // Receiving File Mode
    int filemode;
    int modemsg;
    if ((modemsg = recv(recfd, &filemode, sizeof(filemode), 0)) == -1) {
      perror("main mode recv");
    }
    int mode = ntohl(filemode);
    data->filemode = mode;
    printf("Received: Filemode %d from %s\n", mode, currentAddr);

    // Receiving the first message from the client
    int msg_type;
    int msg;
    if ((msg = recv(recfd, &msg_type, sizeof(int), 0)) == -1) {
      perror("main msg_type recv");
      exit(1);
    }
    data->msg_type = ntohl(msg_type);
    printf("Received: %x from %s\n", data->msg_type, currentAddr);

    // Threading stuff. Should create a worker thread to pass data to msg_handler.
    pthread_t child;
    pthread_create(&child, NULL, msg_handler, (void *)data);
  }
  return 0;
}
