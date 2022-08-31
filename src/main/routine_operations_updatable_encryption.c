
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

char** db_key_wraps;
char** db_key_headers;
char** db_key_ctxts;

CK_BYTE_PTR * wraps;
ct_hat_data_en* ciphertext_hats;
uint8_t ** ciphertexts;

CK_ULONG* wrap_lens;
int* ctxt_lens;

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
int run_workload = 0;
int end_program = 0;
pthread_mutex_t run_workload_lock;

int num_reencrypts = 64;
#define PORT 6000



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
  
// GET IP ADDRESS
void checkHostName(int hostname)
{
    if (hostname == -1)
    {
        perror("gethostname");
        exit(1);
    }
}
void checkHostEntry(struct hostent * hostentry)
{
    if (hostentry == NULL)
    {
        perror("gethostbyname");
        exit(1);
    }
}
void checkIPbuffer(char *IPbuffer)
{
    if (NULL == IPbuffer)
    {
        perror("inet_ntoa");
        exit(1);
    }
} 
// Driver code
char* get_ip_address()
{
    char hostbuffer[256];
    char *IPbuffer;
    struct hostent *host_entry;
    int hostname;
  
    // To retrieve hostname
    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    checkHostName(hostname);
  
    // To retrieve host information
    host_entry = gethostbyname(hostbuffer);
    checkHostEntry(host_entry);
  
    // To convert an Internet network
    // address into ASCII string
    IPbuffer = inet_ntoa(*((struct in_addr*)
                           host_entry->h_addr_list[0]));
  
    return IPbuffer;
}



void timeron (int interval) {
    sleep (interval);
    time_expired = 1;
}


void TimerSet (int interval) {
    pthread_t thread1;
    int iret1 = pthread_create( &thread1, NULL, timeron, interval);
}


int commandListener(int* sockfd) {

    int connlen;
    struct sockaddr_in cli;
    connlen = sizeof(cli);

    char* recvbuff = (char*)malloc (MAX);

    int connfd;

    // Listen For Start of Round and then Perform full rotation on db
    for(;;) {

        connfd = accept(*sockfd, (SA*)&cli, &connlen);
        if (connfd < 0) {
            printf("server accept failed...\n");
            //exit(0);
        }
        else {
            //printf("server accept the client...\n");
        }

        recvall(connfd, recvbuff, MAX);
        //printf("RAW COMMAND: %.5s\n", recvbuff);

        int number_of_splits;
        char** command_splits = split_by_space(recvbuff, &number_of_splits);
        char* command = command_splits[0];

        if (strlen(command) == 5 && strncmp(command, "START", 5) == 0) {
            //printf("START RECEIVED\n");
            pthread_mutex_lock(&run_workload_lock);
            run_workload = 1;
            pthread_mutex_unlock(&run_workload_lock);

        } else if (strlen(command) == 4 && strncmp(command, "STOP", 4) == 0) {
            //printf("STOP RECEIVED\n");
            pthread_mutex_lock(&run_workload_lock);
            run_workload = 0;
            pthread_mutex_unlock(&run_workload_lock);

        } else if (strlen(command) == 3 && strncmp(command, "END", 3) == 0) {\
            //printf("END RECEIVED\n");
            pthread_mutex_lock(&run_workload_lock);
            run_workload = 0;
            end_program = 1;
            pthread_mutex_unlock(&run_workload_lock);
        } else if (strlen(command) == 7 && strncmp(command, "ROOTKEY", 7) == 0) {\
            //printf("ROOTKEY RECEIVED\n");
            
            pthread_mutex_lock(&run_workload_lock);
            key_handle = atoi(command_splits[1]);
            pthread_mutex_unlock(&run_workload_lock);

            //printf("ROOTKEY completed\n");
        }

        // Send response back to update coordiantor
        bzero(recvbuff, sizeof(recvbuff));
        char* input = "DONE ";
        strncat(recvbuff, input, strlen(input)+1);
        sendall(connfd, recvbuff, strlen(recvbuff)+1);
    }
}


int main (int argc, char** argv) {

    if (argc < 3) {
        printf("Missing Output.txt. Try <../build/src/main/ciphertext_updates_updatable_encryption arguments.txt Output.txt\n");
    } else if (argc < 2) {
        printf("Missing arguments.txt. Try <../build/src/main/ciphertext_updates_updatable_encryption arguments.txt Output.txt\n");
    }

    ///////SETUP CONNECTION//////////
    // Setup Listener
    int sockfd;
    struct sockaddr_in servaddr;

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

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, commandListener, (void*)&sockfd);
    ///////////////END/////////////////////

    // Benchmark Numbers
    //reuse_message_size = 100000;       // 10 KB
    reuse_message_size = param.data_parameters.message_size;
    reuse_message = malloc(reuse_message_size);
    memset(reuse_message, 'a', reuse_message_size);

    CK_RV rv;
    int rc = EXIT_FAILURE;
    
    struct parameters param = {0};
    init_params(argv[1], &param);
    //update_portnum(portnum + 1);

    args = param.pkcs_parameters;
    //ip_addr = param.redis_parameters.ip_addr;
    portnum = param.redis_parameters.portnum;

    // Session Handler Stuff    
    int initcount = 0;
    rv = pkcs11_initialize(args.library);
    while (CKR_OK != rv) {
        rv = pkcs11_initialize(args.library);

        if (initcount > 10) {
            printf("FAILED: pkcs11_initialization\n");
            return rc;
        }

        initcount += 1;
    }

    rv = pkcs11_open_session(args.pin, &reuse_session);
    while (CKR_OK != rv) {
        reuse_session = NULL;
        rv = pkcs11_open_session(args.pin, &reuse_session);

        if (initcount > 20) {
            printf("FAILED: pkcs11_open_session\n");
            return EXIT_FAILURE;
        }

        initcount += 1;
        sleep(1);
    }



    // Read Output.txt and parse it into two arrays
    FILE *fp;
    fp = fopen(argv[2], "r");
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
    //printf("%d Keys in %s\n", entry_count, argv[2]);

    // use number of keys to generate two arrays. (1) holds all the key names. (2) holds the server ip address
    char** db_key_list = malloc(sizeof(char*) * entry_count);
    char** db_key_ip_addr = malloc(sizeof(char*) * entry_count);

    // seek back to the beginning of the file
    fseek(fp, 0, SEEK_SET);

    // Read from the file and add each entry to our two arrays
    int total_keys = 0;
    while ( getline(&line, &len, fp) != -1) {
        //printf("%s\n", line);

        int split_count;
        char** split_arr = split_by_space(line, &split_count);
        if (split_count == 2) {
            db_key_list[total_keys] = split_arr[0];
            split_arr[1][strlen(split_arr[1])-1] = '\0';
            db_key_ip_addr[total_keys] = split_arr[1];
            // printf("KEY: %s\n", split_arr[0]); 
            // printf("IP ADDR: %s\n", split_arr[1]);
        }
        total_keys += 1;
    }

    // for(int i=0; i < total_keys; i++) {
    //     printf("KEY VALUE: %s\n", db_key_list[i]);
    //     printf("IP ADDR  : %s\n", db_key_ip_addr[i]);
    // }
    fclose(fp);

    printf("Done with keys\n");



     key_handle = 8126470;


    // Update each of this instance's assigned keys
    int starting_key_index = 0;
    int num_of_assigned_keys = total_keys;

    // If first line of arguments is filled, then we modify starting_key_index and num_of_assigned_keys. 


    


    // if Run_Workload is true, Perform full rotation on db

    int download_percentage = 50;
    double download_count = 0;
    double upload_count = 0;

    
    double thisstart = get_time_in_seconds();
    for(;;) {

        while (run_workload == 0) {
            if (end_program == 1) {
                break;
            }
            sleep(1);
        }
    
        while (run_workload == 1) {
            int db_key_index = 0;
            update_ip_addr(db_key_ip_addr[db_key_index]);
            //int rv = update_dek_key ( &reuse_session, database_keys[db_key_index], key_handle);
            
            //rv = updatable_update_dek_and_ciphertext(&reuse_session, key_handle, database_keys[db_key_index], num_reencrypts);

            if (rand() % 100 < download_percentage) {
                download_count += 1;

                char * received;
                int length;
                rv = updatable_download_and_decrypt(&reuse_session, db_key_list[db_key_index], &received, &length);

                if (rv == -1) { 
                    redisContext *conn = NULL;
                    // //int return_val = init_redis (&conn);
                    // set(db_key_wraps[db_key_index], (char*)wraps[db_key_index], wrap_lens[db_key_index], conn);
                    // set(db_key_headers[db_key_index], &(ciphertext_hats[db_key_index]), sizeof (ct_hat_data_en), conn);
                    // set(db_key_ctxts[db_key_index], ciphertexts[db_key_index], ctxt_lens[db_key_index], conn);


                    // repair the ciphertext
                    printf("GOT A RV=-1 ERROR. REPAIRING CIPHERTEXT INSDEAD\n");
                    rv = updatable_encrypt_and_upload(&reuse_session, key_handle, db_key_list[db_key_index], reuse_message, reuse_message_size, 64);

                    //close_redis (conn);
                    total_messages -= 1;
                }
                if (strncmp (reuse_message, received, reuse_message_size) != 0) {
                    printf ("Error with decrypting into original message, not same.\n");
                    printf("Received length is %d\n", length);
                    printf("First 7 chars of this NOT SAME message is: %.7s\n", received);
                    //exit(0);
                }

                // Seems to be old check. dont need anymore i think?
                // if (length != reuse_message_size && strncmp(received, reuse_message, length) != 0) {
                //     printf("Failed\n");
                //     printf("ReuseMessageSize: %d\n", reuse_message_size);
                //     printf("MessageSize: %d\n", length);
                //     exit(0);
                // }

                if (rv != -1) {
                    free(received);
                }


            } else {
                upload_count += 1;

                rv = updatable_encrypt_and_upload(&reuse_session, key_handle, db_key_list[db_key_index], reuse_message, reuse_message_size, 64);
            }
                
            total_messages += 1;
            if (total_messages % 100 == 0) {
                printf ("Message # %d\n", total_messages);
            }
        }


        double thisend = get_time_in_seconds();

        printf("Download Percent: %f\n", (download_count / (download_count + upload_count)));
        printf("Upload Percent: %f\n", (upload_count / (download_count + upload_count)));

        printf ("Time passed:   %f\n", (thisend - thisstart));
        printf ("Num rotations: %d\n", total_messages);
        printf ("[LOG] latency_final: %f\n\n", (double)(thisend-thisstart) / (double) (total_messages)); 
    }
    

}


