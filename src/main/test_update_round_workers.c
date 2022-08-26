
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

#define MAX 16000000 // = 10 MB
//#define MAX 20000
#define PORT 6000
#define ARRAY_SIZE 20
#define SA struct sockaddr



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




int recvall2 (int sockfd, void* recvbuf, int buffsize) {
    //bzero (recvbuf, buffsize);
    int total_bytes = 0;
    int nbytes = 0;

    void* startbuf = recvbuf;

    // printf("buffsize?: %d\n", buffsize);
    // printf("total_bytes: %d\n", total_bytes);


    int index = 0;
    printf ("Before recv\n");
    while (total_bytes < buffsize && (nbytes = recv(sockfd, startbuf, buffsize, 0)) > 0){
        // From here, valid bytes are from recvbuf to recvbuf + nbytes.
        // You could simply fwrite(fp, recvbuf, nbytes) or similar. 
        //printf("-----iteration %d------", index);
        startbuf += nbytes;
        total_bytes += nbytes;
        index += 1;
        // printf("startbuff: %d\n", startbuf);
        // printf("totalbytes: %d\n", total_bytes);
        // printf("index: %d\n", index);
        // printf("nbytes: %d\n", nbytes);
            
        if (((char*)recvbuf)[total_bytes-1] == '\0') {
            printf("hit null terminator\n");
            break;
        }
    }
    ((char*)recvbuf)[total_bytes +1] = 0;
    //printf ("Received: %s\n", (char*)recvbuf);
    return total_bytes;
}

int sendall2 (int sockfd, void* sendbuf, int sendsize) {
    int total_bytes = 0;
    int nbytes = 0;

    void* startbuf = sendbuf;

    int max_send = 50000;
    int current_send = sendsize; 
    if (current_send > max_send) {
        current_send = max_send;
    }

    int i = 0;
    while (sendsize > 0 && (nbytes = send(sockfd, startbuf, current_send, 0)) > 0 ) {
        startbuf += nbytes;
        sendsize -= nbytes;
        total_bytes += nbytes;

        current_send = sendsize; 
        if (current_send > max_send) {
            current_send = max_send;
        }

        i += 1;
    }

    return total_bytes;
}

int myThreadFun(int* t_num) {
    int thread_num = *(t_num);

    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else {
        printf("Socket successfully created..\n");
    }
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }
    else {
        printf("Socket successfully binded..\n");
    }


    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed on port %d...\n", PORT);
        exit(0);
    }
    else {
        printf("Server listening on port %d..\n", PORT);
    }
    len = sizeof(cli);

    char* newbuff = (char*)malloc (MAX);
    bzero(newbuff, MAX);

    printf("Ready to accept connections\n");
    for(;;) {   

        printf("WAITING FOR ACCEPT\n");
        printf("SOCKFD: %d\n", sockfd);
        printf("connfd: %d\n", connfd);
        connfd = accept(sockfd, (SA*)&cli, &len);
        if (connfd < 0) {
            printf("server accept failed...\n");
            //exit(0);
        }

        printf("Connection got?\n");
        printf("MAX: %d\n", MAX);

        recvall2(connfd, newbuff, MAX);

        printf("GET SOMETHING\n");
        printf("buff: %s\n", newbuff);



    }
}

// Driver function
int main()
{


    // for (int i = 0; i < num_threads; i++) {

    //     pthread_t thread_id;
    //     int* thread_num = malloc(sizeof(int));
    //     *thread_num = i;
    //     pthread_create(&thread_id, NULL, myThreadFun, (void*) thread_num);
    // }

    pthread_t thread_id;
    int thread_num = 1;
    pthread_create(&thread_id, NULL, myThreadFun, (void*) &thread_num);

    // Accept the data packet from client and verification
    for(;;) {
        sleep (10);
    }
}