#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT "9000"            // The port the server is running on
#define MAX_DATASIZE 1024      // Maximum data size for the buffer

// Function to connect to the server and send/receive data
int main() {
    int sockfd, numbytes;
    char buf[MAX_DATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;

    // The message to send to the server
    const char *message = "Hello, Server!\n";

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;           // Use IPv4
    hints.ai_socktype = SOCK_STREAM;     // Use TCP stream sockets

    // Get the server address info
    if ((rv = getaddrinfo("127.0.0.1", PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // Loop through the results and connect to the first available
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    freeaddrinfo(servinfo); // Free the address info struct

    // Send the message to the server
    if (send(sockfd, message, strlen(message), 0) == -1) {
        perror("send");
        close(sockfd);
        return 1;
    }

    // Receive the response from the server
    if ((numbytes = recv(sockfd, buf, MAX_DATASIZE - 1, 0)) == -1) {
        perror("recv");
        close(sockfd);
        return 1;
    }

    buf[numbytes] = '\0'; // Null-terminate the received buffer
    printf("Received from server: %s\n", buf); // Print the received message

    close(sockfd); // Close the connection
    return 0;
}
