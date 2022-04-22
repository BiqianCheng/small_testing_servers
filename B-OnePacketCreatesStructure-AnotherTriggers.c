// Server side implementation of UDP client-server model
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define PORT 55555
#define MAXLINE 1024

// Driver code
void evalBuffer(char buffer[], int sockfd, struct sockaddr_in servaddr, struct sockaddr_in cliaddr)
{
    printf(
        "Part B: Client sends 'a' to create a data structure. Client sends 'b' "
        "to trigger the bandwidth bug. \n\n-----------------\nphp client.php "
        "a\n------------------\nthen\n-----------------\nphp client.php "
        "b\n------------------\n");

    char largeReply[] =
        "a1b2c3d4e5f6g7h8i9j10a1b2c3d4e5f6g7h8i9j10a1b2c3d4e5f6g7h8i9j10a1b2c3d"
        "4e5f6g7h8i9j10a1b2c3d4e5f6g7h8i9j10a1b2c3d4e5f6g7h8i9j10a1b2c3d4e5f6g7"
        "h8i9j10a1b2c3d4e5f6g7h8i9j10";
    char smallReply[] = "5";
    // char dataStructure[50] = "";
    char *dataStructure;
    dataStructure = (char *)malloc((strlen(smallReply) + 1) * sizeof(char));
    strcpy(dataStructure, smallReply);
    // printf("dataStrcutre: %s\n",dataStructure);

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // printf("test6");
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // printf("test5");
    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) <
        0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    int len, n, o;

    len = sizeof(cliaddr); // len is value/resuslt
                           // printf("test4");
                           // while (1)
                           // {

    // n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);

    if (strcmp(buffer, "a") == 0)
    {
        time_t mytime = time(NULL);
        char *time_str = ctime(&mytime);
        // time_str[strlen(time_str) - 1] = '\0';
        // printf("A: %d\n", strlen(time_str));
        dataStructure = (char *)realloc(dataStructure, (strlen(time_str) + strlen(dataStructure) + 1) * sizeof(char));
        strcat(dataStructure, time_str);
        // printf("Data Structure : %s\n", dataStructure);
        // printf("***DATA STRUCTURE SET***\n");
        // printf("size data: %d\n", sizeof(*dataStructure));
    }
    else
    {
        // dataStructure = (char *)realloc(dataStructure, (strlen(dataStructure)+1)*sizeof(char));
        // printf("Data Structure : %s\n", dataStructure);
        long size = sendto(sockfd, (const char)dataStructure, sizeof(*dataStructure),
                           MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
        // printf("size reply: %d\n", sizeof(*dataStructure));
        // printf("***SENT***\n");
    }

    if (strcmp(buffer, "a") == 0)
    {
        time_t mytime = time(NULL);
        char *time_str = ctime(&mytime);
        // time_str[strlen(time_str) - 1] = '\0';
        // printf("A: %d\n", strlen(time_str));
        dataStructure = (char *)realloc(dataStructure, (strlen(time_str) + strlen(dataStructure) + 1) * sizeof(char));
        strcat(dataStructure, time_str);
        // printf("Data Structure : %s\n", dataStructure);
        // printf("***DATA STRUCTURE SET***\n");
        // printf("size data: %d\n", sizeof(*dataStructure));
    }
    else
    {
        // dataStructure = (char *)realloc(dataStructure, (strlen(dataStructure)+1)*sizeof(char));
        // printf("Data Structure : %s\n", dataStructure);
        long size = sendto(sockfd, (const char)dataStructure, sizeof(*dataStructure),
                           MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
        // printf("size reply: %d\n", sizeof(*dataStructure));
        // printf("***SENT***\n");
    }

    // }

    // return 0;
}

int main()
{
    char buffer[MAXLINE];
    // printf("The size of buffer is %zu\n", sizeof(buffer));
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    klee_make_symbolic(&buffer, sizeof(buffer), "buffer");
    klee_assume(buffer[MAXLINE - 1] == '\0');
    klee_make_symbolic(&sockfd, sizeof(sockfd), "sockfd");
    evalBuffer(buffer, sockfd, servaddr, cliaddr);
    return 0;
}