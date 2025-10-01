#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define PORT 9000
#define BUFF_SZ 1024

volatile int serverRunning = 1;

void createSockDataFile() 
{
    FILE *fp;

    // Make sure the directory exists
    struct stat st = {0};
    if (stat("/var/tmp", &st) == -1) {
        if (mkdir("/var/tmp", 0777) == -1) {
            mkdir("/var", 0777);
            mkdir("/var/tmp", 0777);
        }
    }
}

int acceptConnection(int daemonMode)
{
    int sock, clientSock;
    struct sockaddr_in addr, remoteAddr;
    char buff[BUFF_SZ] = {0};
    size_t charsAdded = 0;

    // Create the in socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket creation failed");
        return 1;
    }

    // Set up the address struct
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    // Bind the socket
    if(bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
    }

    // Daemonize if needed
    if (daemonMode && daemon(0, 0) < 0) {
        perror("daemon");
        close(clientSock);
        exit(EXIT_FAILURE);
    }

    while(serverRunning) {
        // Attempt to listen for incoming connections
        if(listen(sock, 5) < 0) {
            perror("listen failed");
            close(sock);
            return 1;
        }

        // Accept a connection
        clientSock = accept(sock, (struct sockaddr*)&addr, (socklen_t*)&addr);
        if(clientSock < 0) {
            perror("accept failed");
            close(clientSock);
            return 1;
        }

        // Log that the connection has been made
        socklen_t addr_len = sizeof(remoteAddr);
        if(getpeername(clientSock, (struct sockaddr*)&remoteAddr, &addr_len) == -1) {
            perror("getpeername failed");
            close(clientSock);
            close(sock);
            return 1;
        }

        char remoteIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &remoteAddr.sin_addr, remoteIp, sizeof(remoteIp));
        openlog("aesdsocket", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "Accepted connection from %s", remoteIp);
        closelog();
        createSockDataFile();
        FILE *fp = fopen("/var/tmp/aesdsocketdata", "a+");
        
        // Receive data while there's data to read -
        while (1) {
            int rd = recv(clientSock, buff, BUFF_SZ, 0);
            if (rd < 0) {
                perror("recv failed");
                close(clientSock);
                return 1;
            } else if (rd == 0) {
                // Connection closed
                break;
            } else {
                // Write to file
                fwrite(buff, sizeof(char), rd, fp);
                charsAdded += rd;
                printf("Received %d bytes: %.*s", rd, rd, buff);
                // Check for newline to end
                if (buff[rd - 1] == '\n') {
                    break;
                }
            }
        }

        // Send back the file contents
        fseek(fp, 0, SEEK_SET);
        // fseek(fp, -charsAdded, SEEK_END);
        while (fgets(buff, BUFF_SZ, fp) != NULL) {
            printf("sending: %s", buff);
            // send(clientSock, buff, BUFF_SZ, 0);
            send(clientSock, buff, strlen(buff), 0);
        }

        fclose(fp);
        close(clientSock);

        // Log that the connection has been closed
        openlog("aesdsocket", LOG_PID, LOG_USER);
        syslog(LOG_INFO, "Closed connection from %s", remoteIp);
        closelog();
    }

    // Close connection
    close(sock);
}

// To be used as the signal handler for SIGINT and SIGTERM 
void terminateGracefully(int signum) 
{
    // Log that the signal was caught
    openlog("aesdsocket", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "Caught signal, exiting");
    closelog();

    // Set the running flag to false and clean up
    serverRunning = 0;
}

int main(int argc, char *argv[]) 
{
    printf("main1\n");
    // Make sure that external signals to terminate are handled gracefully
    struct sigaction gracefulTerm;
    memset(&gracefulTerm, 0, sizeof(gracefulTerm));
    gracefulTerm.sa_handler = terminateGracefully;
    sigaction(SIGINT, &gracefulTerm, NULL);
    sigaction(SIGTERM, &gracefulTerm, NULL);

    printf("main2\n");

    // Continually accept connections
    if(argc > 1 && strcmp(argv[1], "-d") == 0) {
        (void) acceptConnection(1);
    } else {
        (void) acceptConnection(0);
    }
        
    remove("/var/tmp/aesdsocketdata");
    printf("main3\n");

    return 0;
}