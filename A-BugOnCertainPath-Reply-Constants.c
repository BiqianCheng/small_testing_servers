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
void evalBuffer(char buffer[], int sockfd, struct sockaddr_in servaddr, struct sockaddr_in cliaddr, char reply[])
{
    printf("Part A: Bandwidth bug on only certain path. Only triggers the bug with client sending a single char 'a'. \n\n-----------------\nphp client.php a\n------------------\n");

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

    n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL,
                 (struct sockaddr *)&cliaddr, &len);

    if (strcmp(buffer, "a") == 0)
    {
        printf("size of reply: %lu\n", sizeof(*reply)/sizeof(char));
        
        if (sizeof(reply) > 100)
        {
            sendto(sockfd, (const char *)reply, sizeof(*reply),
                   MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
        }
        else
        {
            sendto(sockfd, (const char *)reply, sizeof(*reply),
                   MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
        }

        // long size = sendto(sockfd, (const char *)largeReply, sizeof(largeReply),
        //                    MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
        printf("\n -------------- \n");
    }
    else
    {
        // long size = sendto(sockfd, (const char *)smallReply, sizeof(smallReply),
        //                    MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
        sendto(sockfd, (const char *)reply, sizeof(*reply),
               MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
        // klee_print_expr("buffer != a", *buffer);
    }
    // }
}

int main()
{
    char buffer[MAXLINE];
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char reply[MAXLINE];
    // char *reply = malloc(MAXLINE);
    klee_make_symbolic(&reply, sizeof(reply), "reply");
    klee_make_symbolic(&buffer, sizeof(buffer), "buffer");
    klee_assume(buffer[MAXLINE - 1] == '\0');
    klee_make_symbolic(&sockfd, sizeof(sockfd), "sockfd");
    evalBuffer(buffer, sockfd, servaddr, cliaddr, reply);

    return 0;
}


