#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include "server.h"

#define MAX_SIZE 16000

int server_tcp(int sockfd)
{
    struct sockaddr_storage client_addr;
    socklen_t client_addrlen;
    int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addrlen);
    if (clientfd == -1) {
        perror("accept");
        return -2;
    }

    ssize_t r;
    size_t length = 0, recv_bytes = 0;
    do {
        r = recv(clientfd, ((void*)(&length) + recv_bytes), sizeof(uint32_t) - recv_bytes, 0);
        if (r == -1) {
            perror("recv");
            close(clientfd);
            return -2;
        }
        recv_bytes += r;
    } while (recv_bytes != sizeof(uint32_t));
    length = ntohl(length); // length now contains the number of 32-bits integers to read.

    size_t bufsize = length * sizeof(uint32_t);
    uint32_t *buf = malloc(bufsize);
    if (buf == NULL) {
        close(clientfd);
        perror("malloc");
        return -1;
    }

    recv_bytes = 0;
    do {
        r = recv(clientfd, ((void*)(buf) + recv_bytes), (bufsize - recv_bytes), 0);
        if (r == -1) {
            perror("recv");
            close(clientfd);
            free(buf);
            return -2;
        }
        recv_bytes += r;
    } while (recv_bytes != bufsize);

    uint32_t sum = 0;
    uint32_t prefix_sum[length];

    for (unsigned i = 0; i < length; i++) {
        sum += ntohl(buf[i]);
        if (i == 0)
            prefix_sum[i] = ntohl(buf[i]);
        else
            prefix_sum[i] = ntohl(buf[i]) + prefix_sum[i - 1];
    }

    for (unsigned i = 0; i < length; i++) {
        prefix_sum[i] = htonl(prefix_sum[i]); // converts the unsigned integer hostlong from host byte order to network byte order.
    }

    free(buf); // deallocates the memory previously allocated by a call to calloc, malloc, or realloc
    sum = htonl(sum); // converts the unsigned integer hostlong from host byte order to network byte order.

    ssize_t s = 0;
    size_t sent_bytes = 0;
    do {
        /*
         * 	 * The send() function sends data on the socket with descriptor socket. The send() call applies to all connected sockets.
             * Params
             * socket: The socket descriptor.
             * msg: The pointer to the buffer containing the message to transmit.
             * length: The length of the message pointed to by the msg parameter.
             * flags
                The flags parameter is set by If more than one flag is specified, the logical OR operator (|) must be used to separate them.
                MSG_OOB
                    Sends out-of-band data on sockets that support this notion. Only SOCK_STREAM sockets support out-of-band data.
                    The out-of-band data is a single byte.
                    Before out-of-band data can be sent between two programs, there must be some coordination of effort.
                    If the data is intended to not be read inline, the recipient of the out-of-band data must specify the recipient
                    of the SIGURG signal that is generated when the out-of-band data is sent. If no recipient is set, no signal is sent.
                    The recipient is set up by using F_SETOWN operand of the fcntl command, specifying either a pid or gid.
                    For more information on this operand, refer to the fcntl command.
                    The recipient of the data determines whether to receive out-of-band data inline or not inline by the setting
                    of the SO_OOBINLINE option of setsockopt(). For more information on receiving out-of-band data,
                    refer to the setsockopt(), recv(), recvfrom() and recvmsg() commands.
                MSG_DONTROUTE: The SO_DONTROUTE option is turned on for the duration of the operation.
                This is usually used only by diagnostic or routing programs.
                If successful, send() returns 0 or greater indicating the number of bytes sent.
                However, this does not assure that data delivery was complete.
                A connection can be dropped by a peer socket and a SIGPIPE signal generated at a
                later time if data delivery is not complete.
                If unsuccessful, send() returns -1 indicating locally detected errors and sets errno
                to one of the following values. No indication of failure to deliver is implicit in a send() routine.
         */
        s = send(clientfd, ((void*)(&sum) + sent_bytes), sizeof(uint32_t) - sent_bytes, 0);
        if (s == -1) {
            perror("send");
            close(clientfd);
            return -2;
        }
        sent_bytes += s;
    } while (sent_bytes != sizeof(uint32_t));

    size_t s1 = 0;
    size_t sent_bytes_prefix_sum = 0;
    do {
        s1 = send(clientfd, ((void*) (&prefix_sum) + sent_bytes_prefix_sum), (sizeof(uint32_t) * length - sent_bytes_prefix_sum), 0);
        if (s1 == -1) {
            perror("send prefix sum");
            close(clientfd);
            return -2;
        }
        sent_bytes += s1;
    } while (sent_bytes_prefix_sum != sizeof(uint32_t) * length);

    if (close(clientfd) == -1) {
        perror("close");
        return -2;
    }
    return 0;
}

int main(int argc, char *argv[])
{
  struct sockaddr_in servaddr, cliaddr;
  int sockfd;

  if (argc < 2) {
    printf("Uso: ./server <port>\n");
    exit(1);
  }
  
  short int port_number;
  if (sscanf(argv[1], "%hu", &port_number) != 1) {
    printf("Error converting the port number.\n");
    exit(1);
  }
  printf("Usando a porta %hu\n", port_number);
  
  // Creating socket file descriptor 
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) { 
    perror("socket creation failed"); 
    exit(EXIT_FAILURE); 
  } 
      
  memset(&servaddr, 0, sizeof(servaddr)); 
  memset(&cliaddr, 0, sizeof(cliaddr)); 
      
  // Filling server information 
  servaddr.sin_family    = AF_INET; // IPv4 
  servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); //INADDR_ANY;
  servaddr.sin_port = htons(port_number); 
  
  // Bind the socket with the server address 
  if ( bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) 
    { 
      perror("bind failed"); 
      exit(EXIT_FAILURE); 
    }
  listen(sockfd, 5);
  while (1)
    printf("Status: %d\n", server_tcp(sockfd));

  close(sockfd);
}

