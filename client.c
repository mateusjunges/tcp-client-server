#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#include "client.h"

#define MAX_SIZE 16000

/**
 *
 * @param sockfd
 * @param tab
 * @param length
 * @param rep
 * @return
 */
int get_sum_of_ints_tcp(int sockfd, uint32_t *tab, size_t length, uint32_t *rep)
{
    ssize_t s, sent_bytes = 0, recv_bytes = 0, total_size = (length + 1) * sizeof(uint32_t);
    uint32_t *newtab = malloc(total_size);
    if (newtab == NULL) {
        perror("malloc");
        return -1;
    }

    newtab[0] = htonl(length);
    for (unsigned int i = 0; i < length; i++) {
        newtab[i + 1] = htonl(tab[i]); // converts the unsigned integer hostlong from host byte order to network byte order.
    }
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
        s = send(sockfd, ((void*)(newtab) + sent_bytes), total_size - sent_bytes, 0); //Send a TCP package to the server
        if (s == -1) {
            perror("send");
            free(newtab);
            return -2;
        }
        sent_bytes += s;
    } while (sent_bytes != total_size);

    free(newtab); //deallocates the memory previously allocated by a call to calloc, malloc, or realloc

    // Done with sending, now waiting for an answer.
    uint32_t answer;
    do {
        /* Recv Parameters:
         * sockfd: The socket descriptor
         * answer: The pointer to the buffer that receives the data.
         * len: The length in bytes of the buffer pointed to by the buf parameter
         * flags: Not used, but can receive one or more of the following flags: MSG_CONNTERM|MSG_OOB|MSG_PEEK|MSG_WAITALL
         * */
        s = recv(sockfd, ((void*)(&answer) + recv_bytes), (sizeof(uint32_t) - recv_bytes), 0);
        if (s == -1) {
            perror("recv");
            return -2;
        }
        recv_bytes += s;
    } while (recv_bytes != sizeof(uint32_t));
    *rep = ntohl(answer); //converts the unsigned integer netlong from network byte order to host byte order
    return 0;
}

/**
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{
    int sockfd; 
    struct sockaddr_in   servaddr;
    int status;
    struct hostent *hostinfo;

    // Creating socket file descriptor
    /*Socket params:
     * domain: The address domain requested, either AF_INET, AF_INET6, AF_UNIX, or AF_RAW
     * type: The type of socket created, either SOCK_STREAM, SOCK_DGRAM, or SOCK_RAW
     * The protocol requested. Some possible values are 0, IPPROTO_UDP, or IPPROTO_TCP
     * */
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 

    if (argc < 4) {
      printf("Uso: ./client <server> <port> <integers space separated>\n");
      exit(1);
    }

    long int port_number;
    if (sscanf(argv[2], "%ld", &port_number) != 1) {
      printf("Error converting the port number.\n");
      exit(1);
    }
    hostinfo = gethostbyname(argv[1]); 
    memset(&servaddr, 0, sizeof(servaddr)); 

    // Filling server information
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(port_number);
    memcpy(&servaddr.sin_addr, hostinfo->h_addr_list[0], hostinfo->h_length);

    /*
     * The connect() system call initiates a connection on a socket.
     * int connect(int s, const struct sockaddr *name, socklen_t namelen);
     * Params:
     * s: Specifies the integer descriptor of the socket to be connected.
     * name: Points to a sockaddr structure containing the peer address. The length and format of the address depend on the address family of the socket.
     * namelen: Specifies the length of the sockaddr structure pointed to by name argument.
     * */
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) {
      perror("connect");
      close(sockfd);
      return -1;
    }
    unsigned int result;
    int n = argc - 3; // Vector size.
    int tablen = n*sizeof(uint32_t);
    int l = 0;

    uint32_t tab[n]; //Array of integers. Size of argc - 2, since argv[0] is the program name, argv[1] the server ip and argv[2] the port.
    printf("\nEnviando o array [");
    for (int i=3; i < argc; ++i) { // Get all array elements:
        l = atoi(argv[i]);
        tab[i - 3] = l;
        if (i < argc - 1)
            printf("%d, ", l);
        else printf("%d]", l);
    }

    status = get_sum_of_ints_tcp(sockfd, tab, n, &result);

    if (!status) {
      printf("\nResultado da soma de elementos do array: %u\n", result);
    }  else {
      printf("Erro: %d\n", status);
      return 1;
    }
    uint32_t answer_prefix_sum[n];
    ssize_t s1 = 0;
    ssize_t recv_bytes_prefix_sum = 0;
    do {
        s1 = recv(sockfd, ((void*)(&answer_prefix_sum) + recv_bytes_prefix_sum), (sizeof(uint32_t)*n - recv_bytes_prefix_sum), 0);
        if (s1 == -1){
            perror("Recv prefix sum");
            return -2;
        }
        recv_bytes_prefix_sum += s1;
    } while (recv_bytes_prefix_sum != sizeof(uint32_t) * n);

    printf("Resultado da soma de prefixos: [");

    for (unsigned i = 0; i < n; i++) {
        answer_prefix_sum[i] = ntohl(answer_prefix_sum[i]);
        if (i < n-1) printf("%d, ", answer_prefix_sum[i]);
        else printf("%d]\n", answer_prefix_sum[i]);
    }

}

