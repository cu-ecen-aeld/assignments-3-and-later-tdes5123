#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdatomic.h>

#define PORT 9000
#define BUFF_SZ 1024

typedef struct Node {
    pthread_t connectionWorker;
    struct Node *next;
} Node;

typedef struct ConnectionArgs {
    int sock;
    int clientSock;
} ConnectionArgs;

// volatile int serverRunning = 1;
volatile atomic_int serverRunning = 1;

// Mutexes
pthread_mutex_t fileMutex;
pthread_mutex_t errorMutex;

void createSockDataDir() 
{
    // Make sure the directory exists
    struct stat st = {0};
    if (stat("/var/tmp", &st) == -1) {
        if (mkdir("/var/tmp", 0777) == -1) {
            mkdir("/var", 0777);
            mkdir("/var/tmp", 0777);
        }
    }
}

FILE* openSockDataFile() 
{
    FILE* fp = fopen("/var/tmp/aesdsocketdata", "a+");
    if(errno != 0 || fp == NULL) {
        pthread_mutex_lock(&errorMutex);
        perror("something wrong with file descriptor");
        pthread_mutex_unlock(&errorMutex);
    }
    return fp;
}

void* handleClient(void* args) 
{
    ConnectionArgs* connArgs = (ConnectionArgs*)args;
    struct sockaddr_in remoteAddr;
    char buff[BUFF_SZ] = {0};

    size_t charsAdded = 0;

    // Log that the connection has been made
    socklen_t addr_len = sizeof(remoteAddr);
    if(getpeername(connArgs->clientSock, (struct sockaddr*)&remoteAddr, &addr_len) == -1) {
        pthread_mutex_lock(&errorMutex);
        perror("getpeername failed");
        pthread_mutex_unlock(&errorMutex);
        close(connArgs->clientSock);
        close(connArgs->sock);
        return NULL;
    }

    // Get the remote IP address
    char remoteIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &remoteAddr.sin_addr, remoteIp, sizeof(remoteIp));
    
    // Log that the connection has been accepted (thread safe)
    syslog(LOG_INFO, "Accepted connection from %s", remoteIp);
    
    // Receive data while there's data to read -
    while (serverRunning) {
        int rd = recv(connArgs->clientSock, buff, BUFF_SZ, 0);
        if (rd < 0) {
            pthread_mutex_lock(&errorMutex);
            perror("recv failed");
            pthread_mutex_unlock(&errorMutex);
            close(connArgs->clientSock);
            return NULL;
        } else if (rd == 0) { // Connection closed
            break;
        } else {
            // Write to file
            pthread_mutex_lock(&fileMutex);

            FILE* fp = openSockDataFile();

            fwrite(buff, sizeof(char), rd, fp);
            fclose(fp);
            pthread_mutex_unlock(&fileMutex);

            charsAdded += rd;
            pthread_mutex_lock(&errorMutex);
            printf("Received %d bytes: %.*s", rd, rd, buff);
            pthread_mutex_unlock(&errorMutex);

            // Check for newline to end
            if (buff[rd - 1] == '\n') {
                break;
            }
        }
    }

    // Send back the file contents
        pthread_mutex_lock(&fileMutex);

    FILE* fp = openSockDataFile();
    fseek(fp, 0, SEEK_SET);
    while (fgets(buff, BUFF_SZ, fp) != NULL) {
        pthread_mutex_lock(&errorMutex);
        printf("sending: %s", buff);
        pthread_mutex_unlock(&errorMutex);

        size_t len = strnlen(buff, BUFF_SZ);
        if(len > 0) {
            send(connArgs->clientSock, buff, strlen(buff), 0);
        }
    }
    fclose(fp);
    pthread_mutex_unlock(&fileMutex);


    close(connArgs->clientSock);

    // Log that the connection has been closed
    syslog(LOG_INFO, "Closed connection from %s", remoteIp);

    return NULL;
}

void cleanup(Node* current) {
    Node* next;

    while (current != NULL) {
        next = current->next;  // save next node
        free(current);         // free current node
        current = next;        // move to next node
    }
}

// Joins all server threads in the linked list and cleans up the list
void waitForServerThreadJoins(Node* head) {
    Node* current = head;
    while (current != NULL) {
        pthread_join(current->connectionWorker, NULL);
        Node* temp = current;
        current = current->next;
        free(temp);
    }
}

// Thread function to append timestamps every 10 seconds
void timestampAppender() {
    while (serverRunning) {
        sleep(10);

        // Append timestamp to end of file
        pthread_mutex_lock(&fileMutex);
        FILE *fp = fopen("/var/tmp/aesdsocketdata", "a+");
        if (fp != NULL) {
            char buff[64];
            time_t now = time(NULL);
            strftime(buff, sizeof(buff), "timestamp: %Y-%m-%d %H:%M:%S\n", localtime(&now));
            fputs(buff, fp);
            fclose(fp);
        }
        pthread_mutex_unlock(&fileMutex);
    }
}

int startServer(int daemonMode)
{
    ConnectionArgs connArgs;
    memset(&connArgs, 0, sizeof(connArgs));
    struct sockaddr_in addr;
    Node* head = NULL, *tail = NULL;

    // Create the in socket
    connArgs.sock = socket(AF_INET, SOCK_STREAM, 0);
    if (connArgs.sock < 0) {
        perror("socket creation failed");
        return 1;
    }

    // Set up the address struct
    int opt = 1;
    setsockopt(connArgs.sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    // Bind the socket
    if(bind(connArgs.sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
    }

    // Daemonize if needed
    if (daemonMode && daemon(0, 0) < 0) {
        perror("daemon");
        close(connArgs.clientSock);
        exit(EXIT_FAILURE);
    }

    // Start the timestamp appending thread
    pthread_t timestampThread;
    if (pthread_create(&timestampThread, NULL, (void*)timestampAppender, NULL) != 0) {
        pthread_mutex_lock(&errorMutex);
        perror("pthread_create for timestampAppender failed");
        pthread_mutex_unlock(&errorMutex);
        close(connArgs.clientSock);
        return 1;
    }

    while(serverRunning) {
        // Attempt to listen for incoming connections
        if(listen(connArgs.sock, 5) < 0) {
            perror("listen failed");
            close(connArgs.sock);
            return 1;
        }

        // Accept a connection
        socklen_t addr_len = sizeof(addr);
        connArgs.clientSock = accept(connArgs.sock, (struct sockaddr*)&addr, &addr_len);
        if(connArgs.clientSock < 0) {
            perror("accept failed");
            close(connArgs.clientSock);
            return 1;
        }

        // Handle the client connection
        Node *newNode = (Node*)malloc(sizeof(Node));
        if (newNode == NULL) {
            perror("malloc failed");
            close(connArgs.clientSock);
            continue;
        }
        newNode->connectionWorker = 0;
        newNode->next = NULL;
        if(pthread_create(&newNode->connectionWorker, NULL, handleClient, (void*)&connArgs) != 0) {
            perror("pthread_create failed");
            free(newNode);
            close(connArgs.clientSock);
            continue;
        }

        newNode->next = NULL;

        // Add to linked list
        if (head == NULL) {
            printf("Adding first node\n");
            head = newNode;
            tail = newNode;
        } else {
            printf("Adding node to tail\n");
            tail->next = newNode;
            tail = newNode;
        }

        // Attempt to join finished threads
        Node *current = head;
        Node *prev = NULL;
        while(current != NULL) {

            // Join if finished and remove from list
            printf("Trying to join thread %ld\n", current->connectionWorker);
            // if (pthread_tryjoin_np(current->connectionWorker, NULL) == 0) {
            if (pthread_join(current->connectionWorker, NULL) == 0) {
                if(prev == NULL) {
                    head = current->next;
                } else {
                    prev->next = current->next;
                }

                Node* toFree = current;
                current = current->next;
                free(toFree); 
            } else {
                prev = current;
                current = current->next;
            }
        }

        waitForServerThreadJoins(head);
    }

    // Wait for timestamp thread to finish
    pthread_join(timestampThread, NULL);

    // Cleanup and close server socket
    cleanup(head);
    close(connArgs.sock);
}

// To be used as the signal handler for SIGINT and SIGTERM 
void terminateGracefully(int signum) 
{
    // Log that the signal was caught
    syslog(LOG_INFO, "Caught signal, exiting");

    // Set the running flag to false and clean up
    serverRunning = 0;
}

int main(int argc, char *argv[]) 
{
    createSockDataDir();

    // Initialize mutexes
    pthread_mutex_init(&fileMutex, NULL);
    pthread_mutex_init(&errorMutex, NULL);

    // Make sure that external signals to terminate are handled gracefully
    struct sigaction gracefulTerm;
    memset(&gracefulTerm, 0, sizeof(gracefulTerm));
    gracefulTerm.sa_handler = terminateGracefully;
    sigaction(SIGINT, &gracefulTerm, NULL);
    sigaction(SIGTERM, &gracefulTerm, NULL);

    // Open syslog for initial log
    openlog("aesdsocket", LOG_PID, LOG_USER);
 

    // Continually accept connections
    if(argc > 1 && strcmp(argv[1], "-d") == 0) {
        (void) startServer(1);
    } else {
        (void) startServer(0);
    }
        
    remove("/var/tmp/aesdsocketdata");
    printf("main3\n");
    closelog();

    return 0;
}