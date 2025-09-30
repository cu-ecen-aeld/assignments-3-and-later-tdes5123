#include <arpa/inet.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#define PORT 9000
#define BUFF_SZ 1024

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

    // Create or truncate the file
    fp = fopen("/var/tmp/aesdsocketdata", "w");
    if (fp == NULL) {
        perror("File creation failed");
        exit(1);
    }
    fclose(fp);
}

int main(int argc, char *argv[]) 
{
    int sock, clientSock;
    struct sockaddr_in addr, remoteAddr;
    char buff[BUFF_SZ] = {0};

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

    // Attempt to listen for incoming connections
    if(listen(sock, 5) < 0) {
        perror("listen failed");
        close(sock);
        return 1;
    }

    clientSock = accept(sock, (struct sockaddr*)&addr, (socklen_t*)&addr);
    if(clientSock < 0) {
        perror("accept failed");
        close(sock);
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
    FILE *fp = fopen("/var/tmp/aesdsocketdata", "a");
    
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
            // Check for newline to end
            if (buff[rd - 1] == '\n') {
                break;
            }
        }
    }

    // Send back the file contents
    fseek(fp, 0, SEEK_SET);
    while (fgets(buff, BUFF_SZ, fp) != NULL) {
        send(clientSock, buff, strlen(buff), 0);
    }

    // Close connection
    fclose(fp);
    close(clientSock);
    close(sock);

    // Log that the connection has been closed
    openlog("aesdsocket", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "Closed connection from %s", remoteIp);
    closelog();

    return 0;
}