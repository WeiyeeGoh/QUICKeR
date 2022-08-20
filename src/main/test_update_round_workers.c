
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
#define PORT 7654
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

    char* buff = (char*)malloc (MAX);


    for(;;) {

        connfd = accept(sockfd, (SA*)&cli, &len);
        if (connfd < 0) {
            printf("server accept failed...\n");
            //exit(0);
        }
        else {
            //printf("server accept the client...\n");
        }

        recvall(connfd, buff, MAX);

        // print
        printf("buff: %s\n", buff);


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