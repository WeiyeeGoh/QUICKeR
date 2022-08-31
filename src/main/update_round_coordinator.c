
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
CK_OBJECT_HANDLE key_handle_arr[] = {2097212, 2097213, 2097214};
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

char additional_params[10];

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

int connect_to_server(char* ip_address, char* port_string) {
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

    //convert port to string
    int port = atoi(port_string);


    // assign IP, PORT
    printf("HELLO\n");
    servaddr.sin_family = AF_INET;
    printf("HELLO: %s\n", ip_address);
    printf("HELLO: %d\n", port);
    servaddr.sin_addr.s_addr = inet_addr(ip_address);
    printf("HELLO\n");
    //servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(port);
    printf("HELLO\n");


    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the server failed on port %d...\n", port);
        exit(0);
    }
    else
        printf("connected to the server on port %d..\n", port);

    return sockfd;
}


void send_start(int sockfd)
{
   printf("sockfd: %d\n", sockfd);
    char* buff = malloc (sizeof(char) * MAX);
    bzero(buff, MAX);

    char* input = "START ";
    strncat(buff, input, strlen(input));
    strncat(buff, additional_params, strlen(additional_params) + 1);

    sendall(sockfd, buff, strlen(buff)+1);

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

void send_client_root_key(int sockfd, char* root_key) {
    char* buff = malloc (sizeof(char) * MAX);
    bzero(buff, MAX);
    printf("hello\n");

    char* input = "ROOTKEY ";
    strncat(buff, input, strlen(input));
    strncat(buff, root_key, strlen(root_key) + 1);


    sendall(sockfd, buff, strlen(buff) + 1);

    bzero(buff, MAX);
    recvall(sockfd, buff, MAX);
    printf("hello\n");
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


int read_address_file_to_arr(char* filename, char*** arr1, char*** arr2) {
    FILE *fp;
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("EXIT FAILURE\n");
        exit(EXIT_FAILURE);
    }

    // setup vars for reading each line
    int read = 0;
    char  *line = NULL;
    int len = 0;
    int entry_count = 0;

    // count number of keys in db
    while ( getline(&line, &len, fp) != -1) {
        entry_count += 1;
    }

    // use number of keys to generate two arrays. (1) holds all the key names. (2) holds the server ip address
    *arr1 = malloc(sizeof(char*) * entry_count);
    *arr2 = malloc(sizeof(char*) * entry_count);

    // seek back to the beginning of the file
    fseek(fp, 0, SEEK_SET);

    // Read from the file and add each entry to our two arrays
    int index = 0;
    while ( getline(&line, &len, fp) != -1) {
        printf("%s\n", line);

        int split_count;
        char** split_arr = split_by_space(line, &split_count);
        if (split_count == 2) {
            (*arr1)[index] = split_arr[0];
            split_arr[1][strlen(split_arr[1])-1] = '\0';
            (*arr2)[index] = split_arr[1];
            printf("KEY: %s\n", split_arr[0]); 
            printf("IP ADDR: %s\n", split_arr[1]);
        }
        index += 1;
    }

    return entry_count;
}

void update_additional_param_for_send_start(starting_index, index_offset) {
    bzero(additional_params, 10);
    char i_string[4];
    bzero(i_string, 4);
    sprintf(i_string, "%d", starting_index);
    strncat(additional_params, i_string, strlen(i_string));
    strncat(additional_params, " ", 1);
    bzero(i_string, 4);
    sprintf(i_string, "%d", index_offset);
    strncat(additional_params, i_string, strlen(i_string));
}


int main(int argc, char** argv) {
    
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
    // prereq: coordinator knows all client machines, update machiunes, and db ip address


    // Create Root Key


    // Notify new Root Key to DB




    // Update Machine
    // 1) Listener that waits for command from Coord
    // 2) When cmd received, it'll do all of its assigned updates
    // 3) sends response back to coord when done

    //0) Setup. Unpack clients_machines_address.txt and update_machines_address.txt
    printf("starting setup\n");
    if (argc < 4) {
        printf("ERROR: Need client_machines_address.txt and update_machines_address.txt db_address.txt passed as arguments\n");
        exit(0);
    }
    char** client_machines_address;
    char** client_machines_port;
    char** update_machines_address;
    char** update_machines_port;
    char** db_address;
    char** db_port;
    int client_machine_count = read_address_file_to_arr(argv[1], &client_machines_address, &client_machines_port);
    int update_machine_count = read_address_file_to_arr(argv[2], &update_machines_address, &update_machines_port);
    int db_count = read_address_file_to_arr(argv[2], &db_address, &db_port);
    printf("COmpleted Setup\n");


    // 1) Choose a Root key
    printf("Doing choosing root key\n");
    int total_rounds = 5;

    for (int round=0; round < total_rounds; round++) {

        key_handle = key_handle_arr[round % 3];
        printf("DONE choosing root key\n");

        // Notify new root key to db. Db saves new root key. 
        redisContext *conn = NULL;
        for (int i=0; i < db_count; i++) {
            printf("SENDing new root key to db\n");
            update_ip_addr(db_address[i]);
            update_portnum(db_port[i]);
            send_root_key(key_handle, conn);
            printf("done sending new root key to db\n");
        }


        // Notify new root key to each Client Machine. Client Machine saves new root key. 
        printf("Notify new root key to client machines\n");
        char root_key_string[10];
        sprintf(root_key_string, "%d", key_handle);
        for(int i=0; i < client_machine_count; i++) {
            conn = NULL;
            int sockfd = connect_to_server(client_machines_address[i], client_machines_port[i]);    
            printf("tryna send stuff\n");
            send_client_root_key(sockfd, root_key_string);
            close (sockfd);
        }
        printf("DONE notifying new root key to client machines\n");

        // Notify All Client Machines to start
        printf("start up client machines\n");
        for(int i=0; i < client_machine_count; i++) {
            conn = NULL;
            int sockfd = connect_to_server(client_machines_address[i], client_machines_port[i]);    
            bzero(additional_params, 10);
            send_start(sockfd);
            close (sockfd);
        }
        printf("DONE starting up client machiens\n");


        // Notify All Update Machines to start. Hold connections here until all are done. 
        printf("update machines starting up\n");
        pthread_t* tid = malloc(sizeof(pthread_t) * update_machine_count);
        pthread_t* sockfd_arr = malloc(sizeof(pthread_t) * update_machine_count);
        for (int i = 0; i < update_machine_count; i++) {
            conn = NULL;
            int sockfd = connect_to_server(update_machines_address[i], update_machines_port[i]);
            sockfd_arr[i] = sockfd;
            printf("sockfd: %d\n", sockfd);
            update_additional_param_for_send_start(i, update_machine_count);
            pthread_create(&tid[i], NULL, send_start, sockfd);
        }
        for (int i =0; i < update_machine_count; i++) {
            pthread_join(tid[i], NULL);
            close(sockfd_arr[i]);
        }
        free(tid);
        free(sockfd_arr);
        printf("DONE with starting up update macines\n");

    }

    printf("COMPETED ALL MY ROUNDS (%d)\n", total_rounds);

    return 0;
}
