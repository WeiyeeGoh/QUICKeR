
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

int recvall (int sockfd, void* recvbuf, int buffsize) {
    //bzero (recvbuf, buffsize);
    int total_bytes = 0;
    int nbytes = 0;

    void* startbuf = recvbuf;


    int index = 0;
    //printf ("Before recv\n");
    while (total_bytes < buffsize && (nbytes = recv(sockfd, startbuf, buffsize, 0)) > 0){
        // From here, valid bytes are from recvbuf to recvbuf + nbytes.
        // You could simply fwrite(fp, recvbuf, nbytes) or similar. 
        //printf("-----iteration %d------", index);
        startbuf += nbytes;
        total_bytes += nbytes;
        index += 1;
            
        if (((char*)recvbuf)[total_bytes-1] == '\0') {
            break;
        }
    }
    ((char*)recvbuf)[total_bytes +1] = 0;
    //printf ("Received: %s\n", (char*)recvbuf);
    return total_bytes;
}

int sendall (int sockfd, void* sendbuf, int sendsize) {
    int total_bytes = 0;
    int nbytes = 0;

    void* startbuf = sendbuf;

    int max_send = 50000;
    int current_send = sendsize; 
    if (current_send > max_send) {
        current_send = max_send;
    }

    int i = 0;
    printf ("Before send\n");
    while (sendsize > 0 && (nbytes = send(sockfd, startbuf, current_send, 0)) > 0 ) {
        printf("iteration %d\n", i);
        startbuf += nbytes;
        sendsize -= nbytes;
        total_bytes += nbytes;

        current_send = sendsize; 
        if (current_send > max_send) {
            current_send = max_send;
        }

        printf("iteration %d\n", i);
        printf("nbytes: %d\n", nbytes);
        i += 1;
    }

    printf ("total_bytes: %d\n", total_bytes);
    printf ("nbytes: %d\n", nbytes);
    printf ("sendsize: %d\n", sendsize);
    printf ("Sent: %s\n", (char*) sendbuf);
    return total_bytes;
}

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
    servaddr.sin_addr.s_addr = inet_addr("172.31.24.149");
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


void test_start(int sockfd)
{
   
    char* buff = malloc (sizeof(char) * MAX);
    bzero(buff, MAX);

    char* input = "START ";
    strncat(buff, input, strlen(input) + 1);


    sendall(sockfd, buff, sizeof(buff)+1);

    bzero(buff, MAX);
    recvall(sockfd, buff, MAX);
}

void test_end(int sockfd)
{
   
    char* buff = malloc (sizeof(char) * MAX);
    bzero(buff, MAX);

    char* input = "END ";
    strncat(buff, input, strlen(input) + 1);


    sendall(sockfd, buff, sizeof(buff)+1);

    bzero(buff, MAX);
    recvall(sockfd, buff, MAX);
}

void test_dowork(int sockfd)
{
    printf("hi\n");
    printf("hi\n");
    printf("hi\n");
    printf("hi\n");
    printf("hi\n");
    printf("Number: %d\n", MAX);
    char* buff = malloc (sizeof(char) * MAX);
    int n;
    printf("hi\n");

    bzero(buff, MAX);
    n = 0;

    printf("hi\n");

    char* input = "START ";
    strncat(buff, input, strlen(input) + 1);
    //strncat(buff, key_arr[option], strlen(key_arr[option]));

    printf("fisrt\n");
    printf("strlenbuff: %d\n", strlen(buff)+1);
    printf("second\n");
    printf("sizeofbuff: %d\n", sizeof(buff));

    sendall(sockfd, buff, sizeof(buff)+1);

    bzero(buff, MAX);
    recvall(sockfd, buff, MAX);
    printf("From Server : %s\n", buff);
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

    printf("Before Connect\n");
    int sockfd = connect_to_server(7003);
    printf("Before CMD\n");
    test_end(sockfd);


    return 0;
}
