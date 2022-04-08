// Server side implementation of UDP client-server model
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <klee/klee.h>
#define PORT 55555
#define MAXLINE 1024

// Driver code
void evalBuffer(char buffer[], int sockfd, struct sockaddr_in servaddr, struct sockaddr_in cliaddr)
{
    printf("Part A: Bandwidth bug on only certain path. Only triggers the bug with client sending a single char 'a'. \n\n-----------------\nphp client.php a\n------------------\n");

    char largeReply[] =
        "a1b2c3d4e5f6g7h8i9j10a1b2c3d4e5f6g7h8i9j10a1b2c3d4e5f6g7h8i9j10a1b2c3d4e5f6g7h8i9j10a1b2c3d4e5f6g7h8i9j10a1b2c3d4e5f6g7h8i9j10a1b2c3d4e5f6g7h8i9j10a1b2c3d4e5f6g7h8i9j10";
    char smallReply[] = "5";

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) <
        0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    int len, n, o;

    len = sizeof(cliaddr); // len is value/resuslt

    // for (int i = 0; i < 2; i++)
    // {
    // n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL,
    //              (struct sockaddr *)&cliaddr, &len);
    // buffer[2] = '\0';
    // printf("Client : %s\n", buffer);

    if (strcmp(buffer, "a") == 0)
    {
        // if (klee_is_symbolic(*buffer))
        // {
        //     printf("buffer is symbolic");
        // }

        // printf("Client : %s\n", buffer);
        // long size = sendto(sockfd, (const char *)largeReply, sizeof(largeReply),
        //                    MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
        printf("Buffer triger:%d, size large reply: %d\n", *buffer, sizeof(largeReply));
        // break;
    }
    else
    {
        if (klee_is_symbolic(buffer))
        {
            printf("buffer is symbolic");
        }

        // printf("Client : %s\n",buffer);
        // long size = sendto(sockfd, (const char *)smallReply, sizeof(smallReply),
        //                    MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
        printf("Buffer triger:%d, size small reply: %d\n", *buffer, sizeof(smallReply));
        // break;
    }
    // }
}

int main()
{
    char buffer[MAXLINE];
    // printf("The size of buffer is %zu\n", sizeof(buffer));
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    klee_make_symbolic(&buffer, sizeof(buffer), "buffer");
    // klee_print_expr("ANYTHING", buffer);
    klee_assume(buffer[MAXLINE - 1] == '\0');
    klee_make_symbolic(&sockfd, sizeof(sockfd), "sockfd");
    evalBuffer(buffer, sockfd, servaddr, cliaddr);

    return 0;
}