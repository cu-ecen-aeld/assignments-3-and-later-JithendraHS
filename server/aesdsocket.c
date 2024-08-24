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
#include <fcntl.h>
#include <sys/stat.h>

#define PORT (9000)              // Port number to bind to
#define PORT_TEXT ("9000")       // Port number as a string
#define MAX_DATASIZE (1024)      // Maximum buffer size for data
#define BACKLOG (2)              // Maximum pending connections in the queue
#define FILE_DIR ("/var/tmp/aesdsocketdata")  // File to store received data

volatile sig_atomic_t keep_running = 1; // Flag to control the server loop

// Signal handler for SIGINT and SIGTERM to handle graceful shutdown
void handle_signal(int signal) {
    syslog(LOG_INFO, "Caught signal %d, exiting", signal);
    keep_running = 0;  // Set flag to stop the server loop
    if (remove(FILE_DIR) == 0)
        syslog(LOG_INFO, "File deleted successfully\n"); // Log file deletion
    else
        syslog(LOG_INFO, "Unable to delete the file\n"); // Log error on deletion failure
}

// Function to get the appropriate IP address (IPv4 or IPv6) from a socket structure
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {  // Check if IPv4
        return &(((struct sockaddr_in*)sa)->sin_addr); // Return IPv4 address
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);  // Return IPv6 address
}

void daemonize() {
    pid_t pid = fork();

    if (pid < 0) {
        syslog(LOG_ERR, "Fork failed: %s", strerror(errno));
        exit(EXIT_FAILURE); // If fork failed, exit
    }

    if (pid > 0) {
        // Parent process exits; child process continues
        exit(EXIT_SUCCESS);
    }

    // Create a new session and become the session leader
    if (setsid() < 0) {
        syslog(LOG_ERR, "setsid failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Ignore signal sent from child to parent process
    signal(SIGCHLD, SIG_IGN); // Ignore child processes to prevent zombies
    signal(SIGHUP, SIG_IGN);  // Ignore hang-up signal to prevent terminal signals

    // Fork again to prevent the process from gaining a controlling terminal
    pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "Second fork failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        // Parent process exits, leaving the daemon in the background
        exit(EXIT_SUCCESS);
    }

    // Set file permissions so that files created by the daemon are restricted
    umask(027);

    // Change the working directory to root to avoid directory locking issues
    if (chdir("/") < 0) {
        syslog(LOG_ERR, "chdir failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Close all standard file descriptors (stdin, stdout, stderr)
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Redirect stdin, stdout, and stderr to /dev/null
    open("/dev/null", O_RDWR);  // Redirect stdin
    dup(0);                     // Redirect stdout
    dup(0);                     // Redirect stderr
}


int main(int argc, char *argv[]){
    int sockfd, status, new_fd, numbytes; 
    struct addrinfo hints, *info;         // Address info structs
    int yes = 1;                          // For setsockopt() to allow address reuse
    socklen_t sin_size;                   // Size of client address structure
    struct sockaddr_storage client_addr;  // Connector's address info
    char buf[MAX_DATASIZE];               // Buffer for data
    char s[INET_ADDRSTRLEN];              // String to store client's IP address
    FILE *fptr;                           // File pointer for data file
    int demon_mode=0;

    if(argc == 2 && strcmp(argv[1], "-d") == 0){
        demon_mode = 1;
    }
    // Set up signal handling for SIGINT and SIGTERM
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);  // Handle Ctrl+C
    sigaction(SIGTERM, &sa, NULL); // Handle termination signal

    openlog("Syslog_logging", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1); // Set up syslog for logging

    // Set up address hints for IPv4 TCP connections
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;        // Use IPv4
    hints.ai_socktype = SOCK_STREAM;  // Use TCP

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

    // Set socket options to allow reuse of the local address
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        closelog();
        return -1;
    }

    // Bind socket to the port
    if (bind(sockfd, info->ai_addr, info->ai_addrlen) == -1) {
        close(sockfd);
        closelog();
        perror("server: bind");
        return -1;
    }

    freeaddrinfo(info); // Free the address info struct
    if(demon_mode){
        daemonize();
    }

    // Listen for incoming connections
    if (listen(sockfd, BACKLOG) == -1) {
        close(sockfd);
        closelog();
        perror("listen");
        return -1;
    }

    // Open the data file in write mode
    fptr = fopen(FILE_DIR, "w");
    if (!fptr) {
        syslog(LOG_ERR, "Failed to open file. Exiting.\n");
        close(sockfd);
        closelog();
        return -1;
    }
    fclose(fptr); // Close the file after initialization

    // Main server loop to accept and handle incoming connections
    while(keep_running) {
        sin_size = sizeof(client_addr);
        new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size); // Accept connection
        if (new_fd == -1) {
            close(sockfd);
            closelog();
            perror("accept");
            return -1;
        }

        // Get and log the client's IP address
        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), s, sizeof(s));
        syslog(LOG_INFO, "Accepted connection from %s\n", s);

        // Open the data file in append mode
        fptr = fopen(FILE_DIR, "a+");
        if (!fptr) {
            syslog(LOG_ERR, "Failed to open file. Exiting.\n");
            close(sockfd);
            closelog();
            return -1;
        }

        // Receive data from the client and write it to the file
        while (keep_running) {
            numbytes = recv(new_fd, buf, MAX_DATASIZE - 1, 0); // Receive data from client
            if (numbytes == -1) {
                perror("recv");
                break;
            }

            char *start = buf;
            char *newline = NULL;
            // Check if a newline character exists in the buffer
            if ((newline = strchr(start, '\n')) != NULL) {
                newline++;
                *newline = '\0';  // Null-terminate the data at the newline
                fprintf(fptr, "%s", buf); // Write data to the file
                syslog(LOG_INFO, "Data receive complete\n");
                break;
            } else {
                fprintf(fptr, "%s", buf); // Continue writing data if no newline found
                continue;
            }
        }

        // Send the contents of the file back to the client
        fseek(fptr, 0, SEEK_SET); // Reset file pointer to the beginning
        while (fgets(buf, sizeof(buf), fptr)) { // Read from the file and send to client
            int len = strlen(buf);
            if (send(new_fd, buf, len, 0) == -1) {
                perror("send");
                break;
            }
        }
        syslog(LOG_INFO, "Data send complete\n");
        syslog(LOG_INFO, "Closed connection from %s\n", s);
        fclose(fptr); // Close the file after processing
    }

    // Clean up after the server loop exits
    fclose(fptr);  // Close file pointer
    close(new_fd); // Close client socket
    close(sockfd); // Close server socket
    closelog();    // Close syslog

    return 0;
}
