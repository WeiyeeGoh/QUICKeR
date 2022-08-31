
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
// number of keys per server?
int db_keys_per_server = 100;
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


    double start_time = get_time_in_seconds();


    // Benchmark Numbers
    reuse_message_size = 100000;       // 10 KB
    reuse_message = malloc(reuse_message_size);
    memset(reuse_message, 'a', reuse_message_size);
    reuse_message[reuse_message_size-1] = '\0';
    //printf("%d\n", strlen(reuse_message));

    CK_RV rv;
    int rc = EXIT_FAILURE;
    
    struct parameters param = {0};
    init_params(argv[1], &param);

    args = param.pkcs_parameters;
    portnum = param.redis_parameters.portnum; // keep as is


    // Get list of ip_addresses (HARDCODED NOW)
    char* server_ip_addr_list[] = {"172.31.47.126"};
    int server_count = sizeof(server_ip_addr_list) / sizeof(server_ip_addr_list[0]);
    // char* server_ip_addr_string_list = param.redis_parameters.ip_addr;
    // int server_count;
    // server_ip_addr_list = split_by_comma(server_ip_addr_string_list, &server_count);


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
    int key_handle_list[] = {2097212, 2097213, 2097214};
    //8126470
    key_handle = 2097212;

    // Get Machine IP address 
    char* machine_ip_address  = get_ip_address();
    int ip_addr_size = sizeof(machine_ip_address);

    // Create database_keys space
    int num_of_db_keys = db_keys_per_server * server_count;
    database_keys = (char**) malloc(sizeof(char*) * (num_of_db_keys));
    memset(database_keys, 0, sizeof(char*) * (num_of_db_keys));

    // Generate ciphertexts for all keys    
    // Encrypt Data using Updatable Encryption
    AE_key ae_key;
    AE_KeyGen( & ae_key, 64);

    ct_hat_data_en ciphertext_hat;
    int buffer_length = sizeof(AE_ctx_header) + reuse_message_size + 64 * (2 * RHO + NU);
    uint8_t * ciphertext = (uint8_t * ) malloc(buffer_length);

    int ctx_length = AE_Encrypt(&ae_key, reuse_message, &ciphertext_hat, ciphertext, reuse_message_size);


    // Wrap the Decryption Key  
    CK_BYTE_PTR wrapped_key = NULL;
    CK_ULONG wrapped_len = 0;
    int fail_counter = 0;
    while(1) {
        wrapped_key = NULL;
        wrapped_len = 0;
        rv = hsm_aes_encrypt(reuse_session, key_handle, &ae_key, sizeof(AE_key), &wrapped_key, &wrapped_len);
        
        if (rv == CKR_OK) {
            break;
        }
        fail_counter += 1;
        if (fail_counter > 10) {
            break;
        }
        sleep(1);
    }

    if (rv != CKR_OK) {
        printf("HSM Aes Encrypt Failed\n");
        exit(1);
    }


    char* b64_ciphertext = (char*) base64_enc((char*) ciphertext, ctx_length);
    char* b64_ciphertext_hat = base64_enc((char*) &ciphertext_hat, sizeof(ct_hat_data_en));
    char* b64_wrapped_key = base64_enc((char*) wrapped_key, wrapped_len);

    int key_type_count = 5;
    char** key_type_values = malloc(sizeof(char*) * key_type_count);
    char key_handle_string[15];
    bzero(key_handle_string, 15);
    sprintf(key_handle_string, "%d", key_handle);
    char* b64_key_handle = base64_enc((char*) key_handle_string, 15);
    printf("B64 Key Handle: %s\n", b64_key_handle);

    key_type_values[0] = b64_key_handle;
    key_type_values[1] = "0";
    key_type_values[2] = b64_wrapped_key;
    key_type_values[3] = b64_ciphertext_hat;
    key_type_values[4] = b64_ciphertext;

    printf("ctx_length: %d\n", ctx_length);
    int result_length_2;
    char* b64_dec_wrapped_key = base64_dec(b64_ciphertext, strlen(b64_ciphertext), &result_length_2);
    printf("result_length_2: %d\n", result_length_2);


    printf("b64_wrapped_key: %s\n", b64_wrapped_key);
    printf("b64_ciphertext_hat: %s\n", b64_ciphertext_hat);
    printf("strlen of b64_ciphertext: %d\n", strlen(b64_ciphertext));


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

    char wrap_text[] = "wrap_";
    char header_text[] = "header_";
    char data_text[] = "data_";
    char ctxt_version_number[] = "ctxt_version_";
    char root_key_version_number[] = "root_version_";
    char** key_prefix_list = malloc(sizeof(char*) * key_type_count);

    key_prefix_list[0] = root_key_version_number;
    key_prefix_list[1] = ctxt_version_number;
    key_prefix_list[2] = wrap_text;
    key_prefix_list[3] = header_text;
    key_prefix_list[4] = data_text;


    char listbuf[1000000];


    // Generate list of keys for each database

    for (int i = 0; i < server_count; i++) {
        printf("Sending to server %d\n", i);

        for (int k=0; k < key_type_count; k++) {
            listbuf[0] = '\0';
            for (int j = i; j < num_of_db_keys; j+=server_count) {
                strcat(listbuf, key_prefix_list[k]);
                strcat(listbuf, database_keys[j]);
                if (j + server_count < num_of_db_keys) {
                    strcat(listbuf, " ");
                }
            }

            printf("Updating IP Addr to %d\n", server_ip_addr_list[i]);
            update_ip_addr(server_ip_addr_list[i]);



            printf("Performing Save Data On Key %d\n", k);
            //printf("SAVE DATA\n");
            // Send SAVEDATA to redis (technically simpleserver)
            redisContext *conn = NULL;
            savedata(key_type_values[k], conn);



            printf("Performing Populate Data On Key %d\n", k);
            //printf("POPULATE DATA\n");
            // Send POPULTATE to redis
            conn = NULL;
            populate(listbuf, conn);
        }
    }


    double end_time = get_time_in_seconds();
    printf("Took %f Seconds to Populate %d keys on %d servers. DB should show %d keys including keys for the metadata\n", (end_time - start_time), db_keys_per_server, server_count, num_of_db_keys * 5);


    // write to file the location of each key
    FILE *fp;
    fp = fopen("Output.txt", "w");

    for (int i = 0; i < num_of_db_keys; i++) {
        ip_addr = server_ip_addr_list[i % server_count];
        fprintf(fp, "%s %s\n", database_keys[i], ip_addr);
    } 

    fclose(fp);

    pkcs11_finalize_session(reuse_session);

}


