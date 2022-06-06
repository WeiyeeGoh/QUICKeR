
#include <signal.h>
#include <pthread.h>
#include <actions.h>


#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)


// Array of key_handles
int num_forks = 0;
const int num_keys = 3;
CK_OBJECT_HANDLE key_handle_arr [3];
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
double time_expired = false;




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
    time_expired = true;

}


void TimerSet (int interval) {
    
    pthread_t thread1;
    int iret1 = pthread_create( &thread1, NULL, timeron, interval);

}


int main (int argc, char** argv) {
    int count_forks = 0;
    bool parent = true;
    int pid = NULL;
    while (parent && count_forks < num_forks) {
        count_forks ++;
        pid = fork();
        parent = (pid != 0);
    }

    current_pid = count_forks;
    if (pid != 0) {
        current_pid = 0;
    }


    // Benchmark Numbers
    reuse_message_size = 100000;       // 10 KB
    reuse_message = malloc(reuse_message_size);
    memset(reuse_message, 'a', reuse_message_size);

    CK_RV rv;
    int rc = EXIT_FAILURE;

    printf("Init Params\n");
    
    struct parameters param = {0};
    init_params(argv[1], &param);

    printf("Done Init Param. Beginning PKCS11 Init\n");

    args = param.pkcs_parameters;
    ip_addr = param.redis_parameters.ip_addr;
    portnum = param.redis_parameters.portnum;

    // Session Handler Stuff    
    int initcount = 0;
    rv = pkcs11_initialize(args.library);

    printf("pkcs11 init rv: %d\n", rv);
    while (CKR_OK != rv) {

        rv = pkcs11_initialize(args.library);

        if (initcount > 10) {
            printf("FAILED: pkcs11_initialization\n");
            return rc;
        }

        initcount += 1;
        sleep(1);
    }
    printf("Successfully initialized pkcs11. Took %d tries \n", initcount);

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
    printf("Successfully opened conection to pkcs11\n");
    // Generate K_master Key Handle
    // for (int i = 0; i < num_keys; i++) {
    //     // Initialize Wrapping Key Handle (k-master)
    //     generate_new_k_master(&reuse_session, 16, &(key_handle_arr[i]));   
    // }
    // key_handle = key_handle_arr[0];


    sleep(40 - initcount);
    // HardCode K_master key handle
    key_handle = 8126470;

    printf("Done waiting for pkcs11 to complete. Starting Setup\n");


    // Get Machine IP address 
    char* machine_ip_address  = get_ip_address();
    int ip_addr_size = sizeof(machine_ip_address);

    // Create database_keys space
    database_keys = (char**) malloc(sizeof(char*) * (num_of_db_keys));
    memset(database_keys, 0, sizeof(char*) * (num_of_db_keys));

    // Generate num_of_db_keys number of keys into our database_keys datastructure
    for (int i=0; i < num_of_db_keys; i++) {

        database_keys[i] = (char * ) malloc(sizeof(char) * (key_size + ip_addr_size + 12));
        memset(database_keys[i], 0, sizeof(char) * (key_size + ip_addr_size + 12));

        for (int j=0; j < key_size; j++) {
            database_keys[i][j]= (char) (97 + ((int)(i / (int)(pow(26,j))) % 26));
        }

        database_keys[i][key_size] = '\0';
        strcat(database_keys[i], "_");
        strcat(database_keys[i], machine_ip_address);
	   strcat(database_keys[i], "_");
        char tmpstr[10];
	   memset(tmpstr, 0, 10);
        sprintf(tmpstr, "%d", getpid());
	   strcat(database_keys[i], tmpstr);
    }

    //sleep(30);

    printf("Setting up redis database with session\n");
    
    // Setup Database with num_of_db_keys number of keys and encrypted messages
    setup_redis_with_session(reuse_session, reuse_message_size, num_of_db_keys, database_keys, key_handle);


    printf("Completed seting up redis database with session. Starting Update Process. \n");
    

    //sleep(15);

    //TimerSet (290);
    TimerSet (170);

    double thisstart = get_time_in_seconds();
    while (time_expired == false) {
        
        int db_key_index = total_messages % num_of_db_keys;
        int rv = update_dek_key ( &reuse_session, database_keys[db_key_index], key_handle);
        
	if (rv == -1) {
            
             rv = encrypt_and_upload(&reuse_session, key_handle, database_keys[db_key_index], reuse_message, reuse_message_size);
            
	}
	
	total_messages += 1;
	if (total_messages % 100 == 0) {
            printf ("Message # %d\n", total_messages);
	}
    }
    double thisend = get_time_in_seconds();

    printf ("Time passed:   %f\n", (thisend - thisstart));
    printf ("Num rotations: %d\n", total_messages);
    printf ("[LOG] latency_final: %f\n\n", (double)(thisend-thisstart) / (double) (total_messages)); 

    pkcs11_finalize_session(reuse_session);

}

