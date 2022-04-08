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
int main() {
    printf(
        "Part D: Client sends 'a' to have the server read data from a file. Client sends 'b' to trigger the bandwidth bug.\n"
        "\n-----------------\nphp client.php "
        "a\n------------------\nthen\n-----------------\nphp client.php "
        "b\n------------------\n");
    int sockfd;
    char buffer[MAXLINE];
    klee_make_symbolic(&buffer, sizeof(buffer), "buffer");
    char largeReply[] =
        "a1b2c3d4e5f6g7h8i9j10a1b2c3d4e5f6g7h8i9j10a1b2c3d4e5f6g7h8i9j10a1b2c3d"
        "4e5f6g7h8i9j10a1b2c3d4e5f6g7h8i9j10a1b2c3d4e5f6g7h8i9j10a1b2c3d4e5f6g7"
        "h8i9j10a1b2c3d4e5f6g7h8i9j10";
    char smallReply[] = "5";
    // char dataStructure[50] = "";
    char *dataStructure;
    dataStructure = malloc(sizeof(char));

    struct sockaddr_in servaddr, cliaddr;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;  // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) <
        0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    int len, n, o;

    len = sizeof(cliaddr);  // len is value/resuslt

    while (1) {
        n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL,
                     (struct sockaddr *)&cliaddr, &len);
        // buffer[n] = '\0';
        printf("Client : %s\n", buffer);

        if (strcmp(buffer, "a") == 0) {
            char fileBuff[MAXLINE];
            FILE *f = fopen("data.txt", "r");
            fgets(fileBuff, MAXLINE, f);
            fclose(f);

            dataStructure = (char *)realloc(
                dataStructure, strlen(fileBuff) + strlen(dataStructure));
            strcat(dataStructure, fileBuff);
            printf("***DATA STRUCTURE SET***\n");
        } else {
            // dataStructure = (char *) realloc(dataStructure,
            // sizeof(dataStructure));
            sendto(sockfd, (const char *)dataStructure, strlen(dataStructure),
                   MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
            printf("***SENT***\n");
        }
    }

    return 0;
}
