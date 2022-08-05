
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
int db_keys_per_server = 2;             // number of keys per server?
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


int main (int argc, char** argv) {


    // Benchmark Numbers
    reuse_message_size = 100000;       // 10 KB
    reuse_message = malloc(reuse_message_size);
    memset(reuse_message, 'a', reuse_message_size);

    CK_RV rv;
    int rc = EXIT_FAILURE;
    
    struct parameters param = {0};
    init_params(argv[1], &param);

    args = param.pkcs_parameters;
    portnum = param.redis_parameters.portnum; // keep as is


    // Get list of ip_addresses
    char** server_ip_addr_list;
    char* server_ip_addr_string_list = param.redis_parameters.ip_addr;
    int server_count;
    server_ip_addr_list = split_by_comma(server_ip_addr_string_list, &server_count);


    int initcount = 0;
    rv = pkcs11_initialize(args.library);
    while (CKR_OK != rv) {
        rv = pkcs11_initialize(args.library);

        if (initcount > 10) {
            printf("FAILED: pkcs11_initialization\n");
            return rc;
        }

        initcount += 1;
        sleep(1);
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



    printf("Starting Key Gen\n");
    // // Generate K_master Key Handle
    // for (int i = 0; i < num_keys; i++) {
    //     // Initialize Wrapping Key Handle (k-master)
    //     generate_new_k_master(&reuse_session, 16, &(key_handle_arr[i]));   
    // }
    
    printf("Ending Key Gen\n");


    // HardCode K_master key handle
    //key_handle = key_handle_arr[0];
    key_handle = 8126470;

    // Get Machine IP address 
    char* machine_ip_address  = get_ip_address();
    int ip_addr_size = sizeof(machine_ip_address);

    // Create database_keys space
    int num_of_db_keys = db_keys_per_server * server_count;
    database_keys = (char**) malloc(sizeof(char*) * (num_of_db_keys));
    memset(database_keys, 0, sizeof(char*) * (num_of_db_keys));




    // Generate num_of_db_keys (just the key name) number of keys into our database_keys datastructure
    for (int i=0; i < num_of_db_keys; i++) {

        database_keys[i] = (char * ) malloc(sizeof(char) * (key_size));
        memset(database_keys[i], 0, sizeof(char) * (key_size));

        for (int j=0; j < key_size; j++) {
            database_keys[i][j]= (char) (97 + ((int)(i / (int)(pow(26,j))) % 26));
        }

        database_keys[i][key_size] = '\0';
    }

    // Setup Database with num_of_db_keys number of keys and encrypted messages (and write to file)

    FILE *fp;
    fp = fopen("Output.txt", "w");

    for (int i = 0; i < num_of_db_keys; i++) {

        ip_addr = server_ip_addr_list[i % server_count];
        //database_key_ip_address[i] = server_ip_addr_list[i % server_count];
        rv = updatable_encrypt_and_upload(&reuse_session, key_handle, database_keys[i], reuse_message, reuse_message_size, 64);

        fprintf(fp, "%s %s\n", database_keys[i], ip_addr);

    } 

    fclose(fp);

    pkcs11_finalize_session(reuse_session);

}
