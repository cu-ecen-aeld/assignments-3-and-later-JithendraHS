#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <signal.h>

#define PORT (9000)             
#define PORT_TEXT ("9000")      
#define MAX_DATASIZE (1024)     
#define BACKLOG 2               

volatile sig_atomic_t keep_running = 1; // Flag to control server loop

// Signal handler to catch SIGINT and SIGTERM signals
void handle_signal(int signal) {
    syslog(LOG_INFO, "Caught signal %d, exiting", signal);
    keep_running = 0;  // Stop server on signal
}

// Returns appropriate Internet address (IPv4 or IPv6)
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr); // IPv4 address
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr); // IPv6 address
}

int main(){
	int sockfd, status, new_fd, numbytes; 
	struct addrinfo hints, *info;         
	int yes = 1;                          
	socklen_t sin_size;                   
	struct sockaddr_storage client_addr;  
	char buf[MAX_DATASIZE];               
	char s[INET_ADDRSTRLEN];              
    FILE *fptr;                           

    // Set up signal handling for graceful shutdown
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);  // Handle Ctrl+C
    sigaction(SIGTERM, &sa, NULL); // Handle termination signal

    openlog("Syslog_logging", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1); // Set up syslog

    // Prepare address hints for IPv4 TCP
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

    // Get address info for binding
	if ((status = getaddrinfo(NULL, PORT_TEXT, &hints, &info)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        closelog();
        return -1;
    }

    // Create socket
	if ((sockfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol)) == -1) {
        perror("server: socket");
        closelog();
		return -1;
    }

    // Allow reuse of local addresses
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        closelog();
        return -1;
    }

    // Bind socket to port
	if (bind(sockfd, info->ai_addr, info->ai_addrlen) == -1) {
        close(sockfd);
        closelog();
        perror("server: bind");
		return -1;
    }

	freeaddrinfo(info); // Free address info

    // Listen for incoming connections
    if (listen(sockfd, BACKLOG) == -1) {
        close(sockfd);
        closelog();
        perror("listen");
        return -1;
    }

    // Main server loop
    while(keep_running) {
        sin_size = sizeof(client_addr);
        new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size); // Accept connection
        if (new_fd == -1) {
            close(sockfd);
            closelog();
            perror("accept");
			return -1;
        }

        // Log client's IP address
		inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), s, sizeof(s));
        syslog(LOG_INFO, "Accepted connection from %s\n", s);

        // Open file for writing received data
        fptr = fopen("/var/tmp/aesdsocketdata", "w");
        if (!fptr) {
            syslog(LOG_INFO, "Failed to open file. Exiting.\n");
            close(sockfd);
            close(new_fd);
            closelog();
            return -1;
        }

        // Receive data from client
        while (keep_running) {
            numbytes = recv(new_fd, buf, MAX_DATASIZE - 1, 0); // Receive data
            if (numbytes == -1) {
                perror("recv");
                break;
            }

            buf[numbytes] = '\0';  // Null-terminate received data

            // Write each line of received data to file
            char *start = buf;
            char *newline;
            while ((newline = strchr(start, '\n')) != NULL) {
                *newline = '\0'; 
                fprintf(fptr, "%s\n", start);
                fflush(fptr);
                start = newline + 1;
            }

            // Exit loop if client closes connection
            if (numbytes == 0) {
                syslog(LOG_INFO, "Data receive complete\n");
                fclose(fptr); // Close file after writing
                break;
            }
        }

        // Read back the data and send it to the client
        fptr = fopen("/var/tmp/aesdsocketdata", "r");
        if (!fptr) {
            syslog(LOG_INFO, "Failed to open file for reading. Exiting.\n");
            close(sockfd);
            close(new_fd);
            closelog();
            return -1;
        } else {
            while (fgets(buf, sizeof(buf), fptr)) { // Send data back to client
                int len = strlen(buf);
                if (send(new_fd, buf, len, 0) == -1) {
                    perror("send");
                    break;
                }
            }
            syslog(LOG_INFO, "Data send complete\n");
        }
    }

    // Close everything after server stops
    syslog(LOG_INFO, "Closed connection from %s\n", s);
    fclose(fptr);
    close(new_fd);
	close(sockfd);
    closelog();

	return 0;
}
