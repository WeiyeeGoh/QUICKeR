
#include <stdio.h>
#include <stdlib.h>
#include <common.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <openssl/rand.h>
#include "aes_ctr.h"
#include "aes_gcm.h"
#include "AE_Nested_AES.h"
#include "hsm_aes_gcm.h"
#include "redis_storage.h"
#include <hiredis/hiredis.h>
#include <errno.h>    
#include <unistd.h>

double time_counter_hsm_encryption;
double time_counter_local_encryption;
double time_counter_store_ciphertext; 


double time_counter_encryption_storekey_operation;
double time_counter_encryption_connect_to_pkcs11;
double time_counter_encryption_send_dek_to_hsm;
double time_counter_encryption_wrap_dek;
double time_counter_encryption_encrypt_plaintext;
double time_counter_encryption_store_ciphertext;
double time_counter_decryption_storekey_namegen;
double time_counter_decryption_retrieve_ciphertext;
double time_counter_decryption_decrypt_wrap;
double time_counter_decryption_decrypt_ciphertext;
double time_counter_encryption_fail_time;
double time_counter_decryption_fail_time;

double time_counter_keyupdate_createkey;
double time_reset_connection;
int reset_connection_counter;
double start, end;


struct pkcs_arguments args;

double get_time_in_seconds();

print_as_hex_values(uint8_t* ciphertext, int ciphertext_length);

int reset_connection(CK_SESSION_HANDLE_PTR session);

int reset_time_counters ();

int setup_redis (struct pkcs_arguments args, 
                 int message_size, 
                 int number_messages, 
                 char** database_keys, 
                 CK_OBJECT_HANDLE wrapping_key_handle);

int setup_redis_with_session (CK_SESSION_HANDLE session, 
                             int message_size, 
                             int number_messages, 
                             char** database_keys, 
                             CK_OBJECT_HANDLE wrapping_key_handle);

int encrypt_and_upload( CK_SESSION_HANDLE_PTR session, 
                        CK_OBJECT_HANDLE wrapping_key_handle, 
                        char* ciphertext_id, 
                        char* message, 
                        int message_len);

int download_and_decrypt (  CK_SESSION_HANDLE_PTR session, 
                            char* ciphertext_id, 
                            CK_OBJECT_HANDLE wrapping_key_handle,
                            char** retrieved_message,
                            int* retrieved_message_length );

CK_RV generate_new_k_master(    CK_SESSION_HANDLE_PTR session,
                                CK_ULONG key_length_bytes,
                                CK_OBJECT_HANDLE_PTR key);


CK_RV delete_old_k_master(  CK_SESSION_HANDLE_PTR session,
                            CK_OBJECT_HANDLE old_key);



int update_master_key ( CK_SESSION_HANDLE_PTR session, 
                        char** database_keys, 
                        unsigned int key_count, 
                        CK_OBJECT_HANDLE old_key_handle, 
                        CK_OBJECT_HANDLE *new_wrapping_key);

int update_dek_key ( CK_SESSION_HANDLE_PTR session, 
                        char* database_key, 
                        CK_OBJECT_HANDLE key_handle);

int update_master_and_dek_key ( CK_SESSION_HANDLE_PTR session, 
                        char** database_keys, 
                        unsigned int key_count, 
                        CK_OBJECT_HANDLE old_key_handle, 
                        CK_OBJECT_HANDLE *new_wrapping_key);


int update_dek_key ( CK_SESSION_HANDLE_PTR session, 
                        char* database_key, 
                        CK_OBJECT_HANDLE key_handle);

int update_master_and_dek_key ( CK_SESSION_HANDLE_PTR session, 
                        char** database_keys, 
                        unsigned int key_count, 
                        CK_OBJECT_HANDLE old_key_handle, 
                        CK_OBJECT_HANDLE *new_wrapping_key);

int updatable_encrypt_and_upload(CK_SESSION_HANDLE_PTR session, 
                        	CK_OBJECT_HANDLE wrapping_key_handle, 
				char* ciphertext_id, 
                                char* message, 
                                int message_length, 
                                int total_re_encrypts);

int updatable_download_and_decrypt( CK_SESSION_HANDLE_PTR session, 
                        	    CK_OBJECT_HANDLE wrapping_key_handle,  
				    char* ciphertext_id, 
                                    char** retrieved_message,
                                    int* retrieved_message_length);

int updatable_update_dek_and_ciphertext(CK_SESSION_HANDLE_PTR session, 
                        		CK_OBJECT_HANDLE wrapping_key_handle, 
					char* ciphertext_id, 
                                        int total_re_encrypts);