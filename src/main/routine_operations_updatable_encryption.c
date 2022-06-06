
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
double time_expired = false;

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
    
    struct parameters param = {0};
    init_params(argv[1], &param);

    args = param.pkcs_parameters;
    ip_addr = param.redis_parameters.ip_addr;
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

    sleep(40 - initcount);

    // // Generate K_master Key Handle
    // for (int i = 0; i < num_keys; i++) {
    //     // Initialize Wrapping Key Handle (k-master)
    //     generate_new_k_master(&reuse_session, 16, &(key_handle_arr[i]));   
    // }


    // HardCode K_master key handle
    //key_handle = key_handle_arr[0];
    key_handle = 8126470;


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

   

    db_key_wraps = malloc (num_of_db_keys * sizeof (char*));
    db_key_headers = malloc (num_of_db_keys * sizeof (char*));
    db_key_ctxts = malloc (num_of_db_keys * sizeof (char*));

    wraps = malloc (num_of_db_keys * sizeof (CK_BYTE_PTR));
    ciphertext_hats = malloc (num_of_db_keys * sizeof (ct_hat_data_en));
    ciphertexts = malloc (num_of_db_keys * sizeof (uint8_t*));


    wrap_lens = malloc (num_of_db_keys * sizeof (CK_ULONG));
    ctxt_lens = malloc (num_of_db_keys * sizeof (int));

    // Setup Database with num_of_db_keys number of keys and encrypted messages
    for (int i = 0; i < num_of_db_keys; i++) {
        //rv = updatable_encrypt_and_upload(&reuse_session, key_handle, database_keys[i], reuse_message, reuse_message_size, 64);
        CK_SESSION_HANDLE_PTR session = &reuse_session;
        CK_OBJECT_HANDLE wrapping_key_handle = key_handle;
        char* ciphertext_id = database_keys[i];
        char* message = reuse_message;
        int message_length = reuse_message_size;
        int total_re_encrypts = 64;

        // Setup Keys to Lookup Values on Redis Store
        char wrap_text[] = "wrap_";
        char header_text[] = "header_";
        char data_text[] = "data_";

        char* kv_key_wrap = malloc(strlen(wrap_text) + strlen(ciphertext_id)+1);
        char* kv_key_header = malloc(strlen(header_text) + strlen(ciphertext_id)+1);
        char* kv_key_data = malloc(strlen(data_text) + strlen(ciphertext_id)+1);
        kv_key_wrap[0] = '\0';
        kv_key_header[0] = '\0';
        kv_key_data[0] = '\0';

        strcat(kv_key_wrap, wrap_text);
        strcat(kv_key_wrap, ciphertext_id);
        strcat(kv_key_header, header_text);
        strcat(kv_key_header, ciphertext_id);
        strcat(kv_key_data, data_text);
        strcat(kv_key_data, ciphertext_id);

        // Encrypt Data using Updatable Encryption
        AE_key ae_key[64];
        for (int j = 0; j < 64; j++) {
            AE_KeyGen( & (ae_key[j]), total_re_encrypts);
        }

        ct_hat_data_en ciphertext_hat;
        int buffer_length = sizeof(AE_ctx_header) + message_length + total_re_encrypts * (2 * RHO + NU);
        uint8_t * ciphertext = (uint8_t * ) malloc(buffer_length);
        uint8_t * ciphertext2 = (uint8_t * ) malloc(buffer_length);

        int ctx_length = AE_Encrypt(&ae_key, message, &ciphertext_hat, ciphertext, message_length);
        //printf ("buffer_length = %d\n", buffer_length);
        //printf ("ctx_length 1 = %d\n", ctx_length);

        int reencrypt_count = 64;
        for (int j = 0; j < reencrypt_count; j++) {
            AE_key* ae_key1 = & (ae_key[j]);
            AE_key* ae_key2 = & (ae_key[j+1]);
            
            delta_token_data delta;
            int token_work = AE_ReKeyGen(ae_key1, ae_key2, &ciphertext_hat, &delta);

            ct_hat_data_en ciphertext_hat2;
            ctx_length = AE_ReEncrypt(&delta, &ciphertext_hat, ciphertext, &ciphertext_hat2, ciphertext2, buffer_length-32);
            ciphertext_hat = ciphertext_hat2;
            memcpy (ciphertext, ciphertext2, buffer_length);
            //printf ("ctx_length = %d\n", ctx_length);

        }   

        free (ciphertext2);
        CK_RV rv;

        // Wrap the Decryption Key  
        CK_BYTE_PTR wrapped_key = NULL;
        CK_ULONG wrapped_len = 0;

        rv = hsm_aes_encrypt(*session, wrapping_key_handle, &(ae_key[reencrypt_count]), sizeof(AE_key), &wrapped_key, &wrapped_len);
        
        if (rv != CKR_OK) {
            printf("HSM Aes Encrypt Failed\n");
            free(wrapped_key);
            free(kv_key_wrap);
                free(kv_key_data);
                exit(1);
        }

        /* Upload Header and Ciphertext (no wrap yet) */
        redisContext *conn = NULL;
        //int return_val = init_redis (&conn);
        set(kv_key_wrap, (char*)wrapped_key, wrapped_len, conn);
        set(kv_key_header, &ciphertext_hat, sizeof (ct_hat_data_en), conn);
        set(kv_key_data, ciphertext, buffer_length, conn);

        //close_redis (conn);

        db_key_wraps[i] = kv_key_wrap;
        db_key_headers[i] = kv_key_header;
        db_key_ctxts[i] = kv_key_data;
        //memcpy (db_key_wraps[i], kv_key_wrap, strlen(kv_key_wrap));
        //memcpy (db_key_headers[i], kv_key_header, strlen(kv_key_header));
        //memcpy (db_key_ctxts[i], kv_key_data, strlen(kv_key_data));

        wraps[i] = wrapped_key;
        //memcpy (wraps[i], wrapped_key, sizeof (CK_BYTE_PTR));
        ciphertext_hats[i] = ciphertext_hat;
        ciphertexts[i] = ciphertext;
        //memcpy (ciphertexts[i], ciphertext, sizeof (uint8_t *));

        wrap_lens[i] = wrapped_len;
        ctxt_lens[i] = buffer_length;

        /*
        for (int j = 0; j < 32; j++){
                rv = updatable_update_dek_and_ciphertext(&reuse_session, key_handle, database_keys[i], num_reencrypts);
        }
        */
    } 


    
    sleep(15);

    //TimerSet (270);
    TimerSet (150);

    time_t t;
    srand((unsigned) time(&t));
    int download_percentage = 50;
    double download_count = 0;
    double upload_count = 0;

    
    double thisstart = get_time_in_seconds();
    while (time_expired == false) {
        
        int db_key_index = total_messages % num_of_db_keys;
        //int rv = update_dek_key ( &reuse_session, database_keys[db_key_index], key_handle);
        
        //rv = updatable_update_dek_and_ciphertext(&reuse_session, key_handle, database_keys[db_key_index], num_reencrypts);

        if (rand() % 100 < download_percentage) {
            download_count += 1;

            char * received;
            int length;
            rv = updatable_download_and_decrypt(&reuse_session, key_handle, database_keys[db_key_index], &received, &length);

            if (rv == -1) { 
                redisContext *conn = NULL;
                //int return_val = init_redis (&conn);
                set(db_key_wraps[db_key_index], (char*)wraps[db_key_index], wrap_lens[db_key_index], conn);
                set(db_key_headers[db_key_index], &(ciphertext_hats[db_key_index]), sizeof (ct_hat_data_en), conn);
                set(db_key_ctxts[db_key_index], ciphertexts[db_key_index], ctxt_lens[db_key_index], conn);

                //close_redis (conn);
                total_messages -= 1;
            }
            else if (strncmp (reuse_message, received, reuse_message_size) != 0) {
                printf ("Error with decrypting into original message, not same.\n");
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

            rv = updatable_encrypt_and_upload(&reuse_session, key_handle, database_keys[db_key_index], reuse_message, reuse_message_size, 64);
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

    pkcs11_finalize_session(reuse_session);

}

