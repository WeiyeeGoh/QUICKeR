
#include <signal.h>
#include <pthread.h>
#include <actions.h>


#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)


// Array of key_handles
int num_forks = 0;
const int num_keys = 1;
CK_OBJECT_HANDLE key_handle_arr [1];
CK_OBJECT_HANDLE key_handle;
int num_of_db_keys = 1;
int key_size = 5;
char** database_keys;

char* userpin;
int total_messages = 0;
double latency = 0;
int number_messages = 0;
int num_finished = 0;
CK_SESSION_HANDLE reuse_session;
CK_SESSION_HANDLE reuse_session2;
char* reuse_message;
int reuse_message_size;
int current_pid = NULL;
double time_expired = 0;

int num_reencrypts = 64;


// C program to display hostname
// and IP address
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int connect_to_server(port) {
    int sockfd;
    struct sockaddr_in servaddr, cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    xservaddr.sin_addr.s_addr = inet_addr("172.31.41.106");
    //servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(port);


    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the server failed on port %d...\n", port);
        exit(0);
    }
    else
        printf("connected to the server on port %d..\n", port);

    return sockfd;
}

int main() {
    
    // New Round Loop
    // 1) Create new Root Key with HSM
        // - Loop through 2 or 3 precreated keys
    // 2) Notify new Root Key to DB. DB saves new Root Key
    // 3) Notify each Client of new Root Key. 
        // - Client will use new Root Key to encrypt now. 
        // - Decrypt uses root key id provided by DB
    // 4) Tell all Update machines to start (also tell them what new root key id)
        // - Update machines will go through their list
        // - Update machine tells Update Coord it is done 
    // 5) Coord ends the round
        // - removes old root key id from HSM
        // - notifies DB old root key no longer necessary
    // prereq: coordinator nows all client machines, update machiunes, and db ip address


    // Create Root Key


    // Notify new Root Key to DB




    // Update Machine
    // 1) Listener that waits for command from Coord
    // 2) When cmd received, it'll do all of its assigned updates
    // 3) sends response back to coord when done

    int sockfd = connect_to_server(7654);


    return 0;
}
