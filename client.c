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
        newtab[i + 1] = htonl(tab[i]);
    }
    do {
        s = send(sockfd, ((void*)(newtab) + sent_bytes), total_size - sent_bytes, 0);
        if (s == -1) {
            perror("send");
            free(newtab);
            return -2;
        }
        sent_bytes += s;
    } while (sent_bytes != total_size);

    free(newtab);

    // Done with sending, now waiting for an answer.
    uint32_t answer;
    do {
        s = recv(sockfd, ((void*)(&answer) + recv_bytes), (sizeof(uint32_t) - recv_bytes), 0);
        if (s == -1) {
            perror("recv");
            return -2;
        }
        recv_bytes += s;
    } while (recv_bytes != sizeof(uint32_t));
    *rep = ntohl(answer);




    return 0;
}

int main(int argc, char *argv[])
{
    int sockfd; 
    struct sockaddr_in   servaddr;
    int status;
    struct hostent *hostinfo;
  
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 

    if (argc < 3) {
      printf("Uso: ./cliente <servidor> <porta>\n");
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

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) {
      perror("connect");
      close(sockfd);
      return -1;
    }
    unsigned int resultado;
    int n = argc - 3;  // tamanho do vetor
    int tablen = n*sizeof(uint32_t);
    int l = 0;

    uint32_t tab[n];
    printf("\nEnviando o array [");
    for (int i=3; i < argc; ++i) {
        l = atoi(argv[i]);
        tab[i - 3] = l;
        if (i < argc - 1)
            printf("%d, ", l);
        else printf("%d]", l);
    }

    status = get_sum_of_ints_tcp(sockfd, tab, n, &resultado);

    if (!status) {
      printf("\nResultado da soma de elementos do array: %u\n", resultado);
//      return 0;
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

