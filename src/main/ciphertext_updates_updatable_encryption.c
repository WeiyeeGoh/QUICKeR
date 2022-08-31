
#include <signal.h>
#include <pthread.h>
#include <actions.h>


#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN
#define PORT 7000

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






int main (int argc, char** argv) {

    if (argc < 3) {
        printf("Missing Output.txt. Try <../build/src/main/ciphertext_updates_updatable_encryption arguments.txt Output.txt\n");
    } else if (argc < 2) {
        printf("Missing arguments.txt. Try <../build/src/main/ciphertext_updates_updatable_encryption arguments.txt Output.txt\n");
    }

    CK_RV rv;
    int rc = EXIT_FAILURE;
    
    struct parameters param = {0};
    init_params(argv[1], &param);

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
    printf("%d Keys in %s\n", entry_count, argv[2]);

    // use number of keys to generate two arrays. (1) holds all the key names. (2) holds the server ip address
    char** db_key_list = malloc(sizeof(char*) * entry_count);
    char** db_key_ip_addr = malloc(sizeof(char*) * entry_count);

    // seek back to the beginning of the file
    fseek(fp, 0, SEEK_SET);

    // Read from the file and add each entry to our two arrays
    int total_keys = 0;
    while ( getline(&line, &len, fp) != -1) {
        printf("%s\n", line);

        int split_count;
        char** split_arr = split_by_space(line, &split_count);
        if (split_count == 2) {
            db_key_list[total_keys] = split_arr[0];
            split_arr[1][strlen(split_arr[1])-1] = '\0';
            db_key_ip_addr[total_keys] = split_arr[1];
            printf("KEY: %s\n", split_arr[0]); 
            printf("IP ADDR: %s\n", split_arr[1]);
        }
        total_keys += 1;
    }

    // for(int i=0; i < total_keys; i++) {
    //     printf("KEY VALUE: %s\n", db_key_list[i]);
    //     printf("IP ADDR  : %s\n", db_key_ip_addr[i]);
    // }
    fclose(fp);

    printf("Done with keys\n");


    // HardCode K_master key handle
    //key_handle = key_handle_arr[0];
    key_handle = 8126470;


    // Update each of this instance's assigned keys
    int starting_key_index = 0;
    int assigned_key_index_offset = 1;
    int num_of_assigned_keys = total_keys;

    // If first line of arguments is filled, then we modify starting_key_index and num_of_assigned_keys. 


    // Setup Listener
    int sockfd, connfd, connlen;
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
    connlen = sizeof(cli);

    char* recvbuff = (char*)malloc (MAX);




    // Listen For Start of Round and then Perform full rotation on db
    for(;;) {

        connfd = accept(sockfd, (SA*)&cli, &connlen);
        if (connfd < 0) {
            printf("server accept failed...\n");
            //exit(0);
        }
        else {
            //printf("server accept the client...\n");
        }

        recvall(connfd, recvbuff, MAX);
        printf("RAW COMMAND: %.5s\n", recvbuff);

        int number_of_splits;
        char** command_splits = split_by_space(recvbuff, &number_of_splits);
        char* command = command_splits[0];


        //If command is START, then perform full update on db
        if (strlen(command) == 5 && strncmp(command, "START", 5) == 0) {

            starting_key_index = atoi(command_splits[1]);
            assigned_key_index_offset = atoi(command_splits[2]);
            printf("Staring Index: %d\n", starting_key_index);
            printf("Index Offset: %d\n", assigned_key_index_offset);

            double thisstart = get_time_in_seconds();
            for (int key_index=starting_key_index; key_index < total_keys; key_index+=assigned_key_index_offset) {

                printf("Doing Update %d on KEY %s\n", key_index, db_key_list[key_index]);
                
                int db_key_index = total_messages % num_of_db_keys;
                //int rv = update_dek_key ( &reuse_session, database_keys[db_key_index], key_handle);

                printf("Updating IP Addr to %d\n", db_key_ip_addr[key_index]);
                update_ip_addr(db_key_ip_addr[key_index]);


                // Get value before we update it
                char * received_before;
                int length_before;

                rv = updatable_download_and_decrypt(&reuse_session, db_key_list[key_index], &received_before, &length_before);

                rv = updatable_update_dek_and_ciphertext(&reuse_session, db_key_list[key_index], num_reencrypts);
                
                // Test it works??
                char * received_after;
                int length_after;
                rv = updatable_download_and_decrypt(&reuse_session, db_key_list[key_index], &received_after, &length_after);

                printf("length_after: %d\n", length_after);
                printf("length_before: %d\n", length_before);

                if (strncmp (received_after, received_before, length_before) != 0) {
                   printf ("Error with decrypting into original message, not same.\n"    );
                } else {
                   printf ("INFO Decryption AFTER UPDATE worked\n");
                } 
                free(received_after);
                free(received_before);

                   
              
                total_messages += 1;
                if (total_messages % 100 == 0) {
                   printf ("Message # %d\n", total_messages);
                }

            }
            double thisend = get_time_in_seconds();

            printf ("Time passed:   %f\n", (thisend - thisstart));
            printf ("Num rotations: %d\n", total_messages);
            printf ("[LOG] latency_final: %f\n\n", (double)(thisend-thisstart) / (double) (total_messages)); 



            // Send response back to update coordiantor
            bzero(recvbuff, sizeof(recvbuff));
            char* input = "DONE ";
            strncat(recvbuff, input, strlen(input)+1);
            sendall(connfd, recvbuff, strlen(recvbuff)+1);


        } else if (strlen(command) == 3 && strncmp(command, "END", 3) == 0) {
            printf("Ending Program\n");
            exit(0);
            
            // Send response back to update coordiantor
            bzero(recvbuff, sizeof(recvbuff));
            char* input = "DONE ";
            strncat(recvbuff, input, strlen(input)+1);
            sendall(connfd, recvbuff, strlen(recvbuff)+1);

            break;
        } else {
            printf("Command is not recognized!\n");
            printf("COMMAND: %.5s\n", command);
            exit(0);
        }


    }

    pkcs11_finalize_session(reuse_session);


}




// int amain (int argc, char** argv) {
//     int count_forks = 0;
//     int parent = 1;
//     int pid = NULL;
//     while (parent && count_forks < num_forks) {
//         count_forks ++;
//         pid = fork();
//         parent = (pid != 0);
//     }

//     current_pid = count_forks;
//     if (pid != 0) {
//         current_pid = 0;
//     }


//     // Benchmark Numbers
//     reuse_message_size = 100000;       // 10 KB
//     reuse_message = malloc(reuse_message_size);
//     memset(reuse_message, 'a', reuse_message_size);

//     CK_RV rv;
//     int rc = EXIT_FAILURE;
    
//     struct parameters param = {0};
//     init_params(argv[1], &param);

//     args = param.pkcs_parameters;
//     ip_addr = param.redis_parameters.ip_addr;
//     portnum = param.redis_parameters.portnum;



//     int initcount = 0;
//     rv = pkcs11_initialize(args.library);
//     while (CKR_OK != rv) {
//         rv = pkcs11_initialize(args.library);

//         if (initcount > 10) {
//             printf("FAILED: pkcs11_initialization\n");
//             return rc;
//         }

//         initcount += 1;
//         sleep(1);
//     }


//     rv = pkcs11_open_session(args.pin, &reuse_session);
//     while (CKR_OK != rv) {
//         reuse_session = NULL;
//         rv = pkcs11_open_session(args.pin, &reuse_session);

//         if (initcount > 20) {
//             printf("FAILED: pkcs11_open_session\n");
//             return EXIT_FAILURE;
//         }

//         initcount += 1;
//         sleep(1);
//     }



//     printf("Starting Key Gen\n");
//     // // Generate K_master Key Handle
//     // for (int i = 0; i < num_keys; i++) {
//     //     // Initialize Wrapping Key Handle (k-master)
//     //     generate_new_k_master(&reuse_session, 16, &(key_handle_arr[i]));   
//     // }
    
//     printf("Ending Key Gen\n");


//     sleep(40 - initcount);
//     // HardCode K_master key handle
//     //key_handle = key_handle_arr[0];
//     key_handle = 8126470;

//     // Get Machine IP address 
//     char* machine_ip_address  = get_ip_address();
//     int ip_addr_size = sizeof(machine_ip_address);

//     // Create database_keys space
//     database_keys = (char**) malloc(sizeof(char*) * (num_of_db_keys));
//     memset(database_keys, 0, sizeof(char*) * (num_of_db_keys));



//     // Generate num_of_db_keys number of keys into our database_keys datastructure
//     for (int i=0; i < num_of_db_keys; i++) {

//         database_keys[i] = (char * ) malloc(sizeof(char) * (key_size + ip_addr_size + 12));
//         memset(database_keys[i], 0, sizeof(char) * (key_size + ip_addr_size + 12));

//         for (int j=0; j < key_size; j++) {
//             database_keys[i][j]= (char) (97 + ((int)(i / (int)(pow(26,j))) % 26));
//         }

//         database_keys[i][key_size] = '\0';
//         strcat(database_keys[i], "_");
//         strcat(database_keys[i], machine_ip_address);
// 	   strcat(database_keys[i], "_");
//         char tmpstr[10];
// 	   memset(tmpstr, 0, 10);
//         sprintf(tmpstr, "%d", getpid());
// 	   strcat(database_keys[i], tmpstr);
//     }

//     printf ("Before setup\n");    
//     // Setup Database with num_of_db_keys number of keys and encrypted messages
//     for (int i = 0; i < num_of_db_keys; i++) {
//         rv = updatable_encrypt_and_upload(&reuse_session, key_handle, database_keys[i], reuse_message, reuse_message_size, 64);
//     } 

//     /*
//     rv = updatable_update_dek_and_ciphertext(&reuse_session, key_handle, database_keys[0], num_reencrypts);
    
//     char * received1;
//     int length1;
//     rv = updatable_download_and_decrypt(&reuse_session, key_handle, database_keys[0], &received1, &length1);

//     if (strncmp (reuse_message, received1, reuse_message_size) != 0) {
//         printf ("Error with decrypting into original message, not same.\n");
//         exit(0);
//     }
//     else {
//         printf ("Decryption worked\n");
//     }
//     */    
//     //sleep(15);


//     //TimerSet (290);
//     TimerSet(170);

//     double thisstart = get_time_in_seconds();
//     while (time_expired == 0) {
        
//         int db_key_index = total_messages % num_of_db_keys;
//         //int rv = update_dek_key ( &reuse_session, database_keys[db_key_index], key_handle);
        
//         //printf ("Before update\n");
// 	   rv = updatable_update_dek_and_ciphertext(&reuse_session, database_keys[db_key_index], num_reencrypts);
//         //printf("After Update\n");
	
//     	//char * received;
//     	//int length;
//     	//rv = updatable_download_and_decrypt(&reuse_session, key_handle, database_keys[db_key_index], &received, &length);

//     	//if (strncmp (reuse_message, received, reuse_message_size) != 0) {
//         //    printf ("Error with decrypting into original message, not same.\n"    );
//         //    exit(0);
//         //}
//         //else {
//         //    printf ("INFO Decryption AFTER UPDATE worked\n");
//         //} 
        

// 	   //exit(0);	

//     	if (rv == -1 || total_messages % num_reencrypts == 0) {
//            rv = updatable_encrypt_and_upload(&reuse_session, key_handle, database_keys[db_key_index], reuse_message, reuse_message_size, 64);
//            //rv = encrypt_and_upload(&reuse_session, key_handle, database_keys[db_key_index], reuse_message, reuse_message_size);
    	   
// 	   printf("INFO reencrypting\n");
// 	   if (rv == -1) {
// 	       total_messages -= 1;
// 	   }
//         }
    	
//     	total_messages += 1;
//     	if (total_messages % 100 == 0) {
//            printf ("Message # %d\n", total_messages);
//     	}
//     }
//     double thisend = get_time_in_seconds();

//     printf ("Time passed:   %f\n", (thisend - thisstart));
//     printf ("Num rotations: %d\n", total_messages);
//     printf ("[LOG] latency_final: %f\n\n", (double)(thisend-thisstart) / (double) (total_messages)); 

//     pkcs11_finalize_session(reuse_session);

// }
