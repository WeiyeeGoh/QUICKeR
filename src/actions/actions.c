

#include "actions.h"



double get_time_in_seconds() {
    struct timeval  tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec) + (tv.tv_usec) / 1000000.0 ;
}

// Used for printing ciphertext because it has weird characters
print_as_hex_values(uint8_t* ciphertext, int ciphertext_length) {
    unsigned char *hex_array = NULL;
    bytes_to_new_hexstring(ciphertext, ciphertext_length, &hex_array);
    if (!hex_array) {
        fprintf(stderr, "Could not allocate hex array\n");
        return -1;
    }
    printf("Downloaded ctxt data: %s\n", hex_array);
    if (NULL != hex_array) {
        free(hex_array);
    }
}

int reset_connection(CK_SESSION_HANDLE_PTR session) {
    reset_connection_counter += 1;

    double reset_start, reset_end;
    reset_start = get_time_in_seconds();

    pkcs11_finalize_session(*session);
    CK_RV rv;
    int rc = EXIT_FAILURE;

    rv = pkcs11_initialize(args.library);
    if (CKR_OK != rv) {
        printf("FAILED: pkcs11_initialization\n");
        rv = rc;
        goto done;
    }

    rv = pkcs11_open_session(args.pin, session);
    if (CKR_OK != rv) {
        printf("FAILED: pkcs11_open_session\n");
        rv = rc;
        goto done;
    }
    done:
    reset_end = get_time_in_seconds();
    time_reset_connection += reset_end - reset_start;
    return rv;
}

int reset_time_counters () {
    time_counter_hsm_encryption = 0.0;
    time_counter_local_encryption = 0.0;
    time_counter_store_ciphertext = 0.0; 
    time_counter_encryption_storekey_operation = 0.0;
    time_counter_encryption_send_dek_to_hsm = 0.0;
    time_counter_encryption_wrap_dek = 0.0;
    time_counter_encryption_encrypt_plaintext = 0.0;
    time_counter_encryption_store_ciphertext = 0.0;
    time_counter_decryption_storekey_namegen = 0.0;
    time_counter_decryption_retrieve_ciphertext = 0.0;
    time_counter_decryption_decrypt_wrap = 0.0;
    time_counter_decryption_decrypt_ciphertext = 0.0;
    time_counter_encryption_fail_time = 0.0;
    time_counter_decryption_fail_time = 0.0;
    time_counter_keyupdate_createkey = 0.0;
    return 0;
}

int setup_redis_with_session (CK_SESSION_HANDLE session, int message_size, int number_messages, char** database_keys, CK_OBJECT_HANDLE wrapping_key_handle) {
    // Session Handler Stuff
    CK_RV rv;
    int rc = EXIT_FAILURE;
    
    char* message = malloc(message_size);
    memset(message, 'a', message_size);

    for (int i = 0; i < number_messages; i++) {
        rv = encrypt_and_upload(&session, wrapping_key_handle, database_keys[i] , message, message_size);
        if (rv != CKR_OK) {
            printf("Encrypt and Upload failed for %s\n", database_keys[i]);
        }
    }

    free(message);
    return 0;
}


int setup_redis (struct pkcs_arguments args, int message_size, int number_messages, char** database_keys, CK_OBJECT_HANDLE wrapping_key_handle) {
    // Session Handler Stuff
    CK_SESSION_HANDLE session;
    CK_RV rv;
    int rc = EXIT_FAILURE;

    rv = pkcs11_initialize(args.library);
    if (CKR_OK != rv) {
        printf("FAILED: pkcs11_initialization\n");
        return rc;
    }

    rv = pkcs11_open_session(args.pin, &session);
    if (CKR_OK != rv) {
        printf("FAILED: pkcs11_open_session\n");
        return rc;
    }

    char* message = malloc(message_size);
    memset(message, 'a', message_size);


    for (int i = 0; i < number_messages; i++) {
        rv = encrypt_and_upload(&session, wrapping_key_handle, database_keys[i] , message, message_size);
        if (rv != CKR_OK) {
            printf("Encrypt and Upload failed for %s\n", database_keys[i]);
        }
    }
    
    pkcs11_finalize_session(session);
    return 0;
}



//int encrypt_and_upload(struct pkcs_arguments args, memcached_st *memc, CK_OBJECT_HANDLE wrapping_key_handle, char* ciphertext_id, char* message, int message_len) {
int encrypt_and_upload(CK_SESSION_HANDLE_PTR session, CK_OBJECT_HANDLE wrapping_key_handle, char* ciphertext_id, char* message, int message_len) {
    // Message To Encrypt
    // uint8_t kv_key_wrap [9] = {'a', 'a', 'a', '-', 'w','r','a','p', 0};
    // uint8_t kv_key_data [9] = {'a', 'a', 'a', '-', 'd','a','t','a', 0};

    
    start = get_time_in_seconds();
    char wrap_text[] = "wrap_";
    char data_text[] = "data_";

    char* kv_key_wrap = malloc(strlen(wrap_text) + strlen(ciphertext_id)+1);
    char* kv_key_data = malloc(strlen(data_text) + strlen(ciphertext_id)+1);
    kv_key_wrap[0] = '\0';
    kv_key_data[0] = '\0';

    strcat(kv_key_wrap, wrap_text);
    strcat(kv_key_wrap, ciphertext_id);
    strcat(kv_key_data, data_text);
    strcat(kv_key_data, ciphertext_id);
    end = get_time_in_seconds();
    time_counter_encryption_storekey_operation += (end - start);
            
    // Generate a Decryption Key
    CK_BYTE aes_key[16];
    RAND_bytes(aes_key, 16);
    //CK_BYTE aes_key[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};


    CK_RV rv;
    int rc = EXIT_FAILURE;
    
    start = get_time_in_seconds();

    double failstart = get_time_in_seconds();
    double failcont;
    // Wrap the Decryption Key  
    CK_BYTE_PTR wrapped_key = NULL;
    CK_ULONG wrapped_len = 0;


    int fail_counter = 0;
    while(1) {
        wrapped_key = NULL;
        wrapped_len = 0;
        rv = hsm_aes_encrypt(*session, wrapping_key_handle, aes_key, 16, &wrapped_key, &wrapped_len);
        if (rv == CKR_OK) {
            break;
        }
        fail_counter += 1;
        sleep(1);
        failcont = get_time_in_seconds();
        time_counter_encryption_fail_time += failcont - failstart;
        failstart = failcont;
    }

    if (rv != CKR_OK) {
        printf("HSM Aes Encrypt Failed\n");
        free(wrapped_key);
        free(kv_key_wrap);
        free(kv_key_data);
        return rv;
    }
    end = get_time_in_seconds();
    time_counter_encryption_wrap_dek += ((double) (end - start));

    start = get_time_in_seconds();
    // Encrypt Message using Decryption Key
    int BUF_CONST = 32 + message_len;    
    int buffer_length = BUF_CONST;
    uint8_t * ciphertext = (uint8_t * ) malloc(buffer_length);
    memset(ciphertext, 0, buffer_length);
    AE_ctx_header * ctxt = (AE_ctx_header * ) ciphertext;
    RAND_bytes(ciphertext, IV_LEN);
    gcm_encrypt((uint8_t * )(message), message_len, aes_key, ctxt->iv, IV_LEN, (uint8_t * )(ciphertext+sizeof(AE_ctx_header)), ctxt->tag);

    end = get_time_in_seconds();
    time_counter_encryption_encrypt_plaintext += ((double) (end - start));


    if (wrapped_len <= 0) {
        printf("ERROR: Wrapped Length in Encrypt is %d\n", wrapped_len);
        return rc;
    }
    if (buffer_length <= 0) {
        printf("ERROR: Buffer Length in Encrypt is %d\n", buffer_length);
        return rc;
    }

    start = get_time_in_seconds();
    /* New Redis Stuff */
    redisContext *conn = NULL;
    //int return_val = init_redis (&conn);

    // set(kv_key_wrap, wrapped_key, wrapped_len, conn);
    // set(kv_key_data, ciphertext, buffer_length, conn);

    char* keys[2] = {kv_key_wrap, kv_key_data};
    char* values[2] = {wrapped_key, ciphertext};
    char* value_sizes[2] = {wrapped_len, buffer_length};
    setall(2, keys, values, value_sizes, conn);
    //close_redis (conn);
    /* End New Redis Stuff */
    end = get_time_in_seconds();
    time_counter_encryption_store_ciphertext += ((double) (end - start));


    free(wrapped_key);
    free (ciphertext);
    free(kv_key_wrap);
    free(kv_key_data);
    return 0;
}


int download_and_decrypt (  CK_SESSION_HANDLE_PTR session, 
                            char* ciphertext_id, 
                            CK_OBJECT_HANDLE wrapping_key_handle,
                            char** retrieved_message,
                            int* retrieved_message_length ) {

    //---------------------DECRYPTION---------------------------------
    ////////////////////////////////////////////////////////////////

    start = get_time_in_seconds();
    char wrap_text[] = "wrap_";
    char data_text[] = "data_";

    char* kv_key_wrap = malloc(strlen(wrap_text) + strlen(ciphertext_id)+1);
    char* kv_key_data = malloc(strlen(wrap_text) + strlen(ciphertext_id)+1);
    kv_key_wrap[0] = '\0';
    kv_key_data[0] = '\0';

    strcat(kv_key_wrap, wrap_text);
    strcat(kv_key_wrap, ciphertext_id);
    strcat(kv_key_data, data_text);
    strcat(kv_key_data, ciphertext_id);
    end = get_time_in_seconds();
    time_counter_decryption_storekey_namegen += end - start;

    // Open a Session to HSM
    CK_RV rv;
    int rc = EXIT_FAILURE;

    start = get_time_in_seconds();
    // /* New Redis Stuff */
    redisContext *conn = NULL;
    //int return_val = init_redis (&conn);
    
    char* kv_keys[2];
    kv_keys[0] = kv_key_wrap;
    kv_keys[1] = kv_key_data;
    int* sizes;
    char** downloaded_wrap_and_ciphertext = getall(2, kv_keys, &sizes, conn);
    //printf("addr: %d\n", sizes);
    // printf("Sizes\n");
    // printf("%d\n", sizes[0]);
    // printf("%d\n", sizes[1]);
    int wrap_length = sizes[0];
    int ciphertext_length = sizes[1];
    uint8_t* downloaded_wrap = downloaded_wrap_and_ciphertext[0];
    uint8_t* downloaded_ciphertext = downloaded_wrap_and_ciphertext[1];
    free (downloaded_wrap_and_ciphertext);
    free (sizes);
    /* End New Redis Stuff */

    int retryforctxtlength = 0;
    while ((wrap_length <= 0 || ciphertext_length <= 0) && retryforctxtlength < 10) {
        free(downloaded_wrap);
        free(downloaded_ciphertext);
        //free(downloaded_wrap_and_ciphertext);
        //free(sizes);
        printf("Redo\n");
        retryforctxtlength++; 
        downloaded_wrap_and_ciphertext = getall(2, kv_keys, &sizes, conn);
        wrap_length = sizes[0];
        ciphertext_length = sizes[1];
        downloaded_wrap = downloaded_wrap_and_ciphertext[0];
        downloaded_ciphertext = downloaded_wrap_and_ciphertext[1];
    	free (downloaded_wrap_and_ciphertext);
    	free (sizes);
    }
    if (wrap_length <= 0) {
        printf("Attempting to get %s\n", kv_key_wrap);
        printf("ERROR: Wrap Length is %d\n", wrap_length);
        free(downloaded_wrap);
        free(downloaded_ciphertext);
        free(kv_key_wrap);
        free(kv_key_data);
        return rc;
    }
    //free(downloaded_wrap_and_ciphertext);
    //free(sizes);

    // /* New Redis Stuff */
    // int ciphertext_length;
    // uint8_t* downloaded_ciphertext = get(kv_key_data, conn, &ciphertext_length);
    // //close_redis (conn);


    // /* End New Redis Stuff */
    // end = get_time_in_seconds();
    // time_counter_decryption_retrieve_ciphertext += end - start;
    // retryforctxtlength = 0;
    // while (ciphertext_length <= 0 && retryforctxtlength < 10) {
    //     retryforctxtlength++; 
    //     downloaded_ciphertext = get(kv_key_data, conn, &ciphertext_length);
    // }
    
    
    if (ciphertext_length <= 0) {
    printf("Attempting to get %s\n", kv_key_data);
        printf("ERROR: Ciphertext Length is %d\n", ciphertext_length);
        free(downloaded_wrap);
        free(kv_key_wrap);
        free(kv_key_data);
        free(downloaded_ciphertext);
        return rc;
    }
    
    start = get_time_in_seconds();
    // Decrypt wrap with master key
    CK_BYTE_PTR decryption_key = NULL;
    CK_ULONG decryption_key_length = 0;

    double failstart = get_time_in_seconds();
    double failcont;

    int fail_counter = 0;

    while(1) {
        rv = hsm_aes_decrypt(*session, wrapping_key_handle, downloaded_wrap, wrap_length, &decryption_key, &decryption_key_length);
        if (rv == 7) {
            reset_connection(session);
            continue;
        } else if (rv == CKR_OK) {
            break;
        } else {
            printf("HSM_AES_DECRYPT ERROR\n");
        }
        fail_counter += 1;
        sleep(1);
        failcont = get_time_in_seconds();
        time_counter_decryption_fail_time += failcont - failstart;
        failstart = failcont;
    }

    end = get_time_in_seconds();
    time_counter_decryption_decrypt_wrap += end - start;
    
    start = get_time_in_seconds();
    // Decrypt Ciphertext with Decryption Key
    AE_ctx_header * downloaded_ctxt = (AE_ctx_header * ) downloaded_ciphertext;
    uint8_t * decrypted_message = (uint8_t * ) malloc(ciphertext_length-32);
    int decrypted_message_length = gcm_decrypt((uint8_t * )(downloaded_ciphertext+sizeof(AE_ctx_header)), ciphertext_length-32, (uint8_t * )(downloaded_ctxt->tag), decryption_key, (uint8_t * )(downloaded_ctxt->iv), IV_LEN, decrypted_message);
    end = get_time_in_seconds();
    time_counter_decryption_decrypt_ciphertext += end - start;

    *retrieved_message = decrypted_message;
    *retrieved_message_length = decrypted_message_length;
    free(downloaded_wrap);
    free(kv_key_wrap);
    free(kv_key_data);
    free(downloaded_ciphertext);
    free(decryption_key);

    return 0;
}

CK_RV generate_new_k_master(    CK_SESSION_HANDLE_PTR session,
                                CK_ULONG key_length_bytes,
                                CK_OBJECT_HANDLE_PTR key) {
    // Open a Session to HSM
    CK_RV rv;
    int rc = EXIT_FAILURE;

    // Generate a new K-master Key
    CK_MECHANISM mech;

    mech.mechanism = CKM_AES_KEY_GEN;
    mech.ulParameterLen = 0;
    mech.pParameter = NULL;

    CK_ATTRIBUTE template[] = {
            {CKA_TOKEN,     &true_val,         sizeof(CK_BBOOL)},
            {CKA_VALUE_LEN, &key_length_bytes,  sizeof(key_length_bytes)},
            {CKA_ENCRYPT,   &true_val,  sizeof(CK_BBOOL)},
            {CKA_DECRYPT,   &true_val,  sizeof(CK_BBOOL)}
    };
    rv = funcs->C_GenerateKey(*session, &mech, template, sizeof(template) / sizeof(CK_ATTRIBUTE), key);

    int counter = 0;
    while  (CKR_OK != rv) {
        rv = funcs->C_GenerateKey(*session, &mech, template, sizeof(template) / sizeof(CK_ATTRIBUTE), key);
        sleep((float)(rand() % 100 + 1)/100.0);
        counter += 1;
        if (counter > 10) {
            break;
        }
    }

    return rv;
}


CK_RV delete_old_k_master(  CK_SESSION_HANDLE_PTR session,
                            CK_OBJECT_HANDLE old_key) {
    // Open a Session to HSM
    CK_RV rv;
    //CK_SESSION_HANDLE session;
    //int rc = EXIT_FAILURE;

    // rv = pkcs11_initialize(args.library);
    // if (CKR_OK != rv) {
    //     return rc;
    // }

    // rv = pkcs11_open_session(args.pin, &session);
    // if (CKR_OK != rv) {
    //     return rc;
    // }

    rv = funcs->C_DestroyObject(*session, old_key);

    //pkcs11_finalize_session(session);

    return rv;
}

int update_master_key ( CK_SESSION_HANDLE_PTR session, 
   //                     memcached_st *memc, 
                        char** database_keys, 
                        unsigned int key_count, 
                        CK_OBJECT_HANDLE old_key_handle, 
                        CK_OBJECT_HANDLE *new_wrapping_key) {

    start = get_time_in_seconds();
    CK_RV rv;
    int rc = EXIT_FAILURE;
    generate_new_k_master(session, 16, new_wrapping_key);
    end = get_time_in_seconds();
    time_counter_keyupdate_createkey += end - start;


    for (unsigned int i=0; i< key_count; i++) {
        char* key_id = database_keys[i];
        char* message;
        int message_length = 0;
        // rv = download_and_decrypt (session, key_id, old_key_handle, &message, &message_length );
        // if (rv != CKR_OK) {
        //     continue;
        // }
        

        // /* New Redis Stuff */
        redisContext *conn = NULL;
        //int return_val = init_redis (&conn);
        int wrap_length;
        char wrap_text[] = "wrap_";

        char* kv_key_wrap = malloc(strlen(wrap_text) + strlen(key_id) + 1);
        

        kv_key_wrap[0] = '\0';
        strcat(kv_key_wrap, wrap_text);
        strcat(kv_key_wrap, key_id);


        start = get_time_in_seconds();
        uint8_t* downloaded_wrap = get(kv_key_wrap, conn, &wrap_length);

        if (wrap_length <= 0) {
            printf("ERROR: Wrap Length is %d\n", wrap_length);
            printf("Downloaded wrap is %s\n", downloaded_wrap);
	    free(downloaded_wrap);
            free(kv_key_wrap);
            return rc;
        }
        end = get_time_in_seconds();
        time_counter_decryption_retrieve_ciphertext+= end - start;


        // Decrypt wrap with master key
        start = get_time_in_seconds();
        CK_BYTE_PTR decryption_key = NULL;
        CK_ULONG decryption_key_length = 0;
        double failcont = 0.0;
        double failstart = start;
        int fail_counter = 0;
        while(1) {
            rv = hsm_aes_decrypt(*session, old_key_handle, downloaded_wrap, wrap_length, &decryption_key, &decryption_key_length);
            if (rv == CKR_OK) {
                break;
            }            
            fail_counter += 1;
            sleep(1);
            failcont = get_time_in_seconds();
            time_counter_decryption_fail_time += failcont - failstart;
            failstart = failcont;
        }
        end = get_time_in_seconds();
        time_counter_decryption_decrypt_wrap += end - start;

        if (rv != CKR_OK) {
            return rc;
        }
        
        // Re-Encrypt Wrap with new master key
        start = get_time_in_seconds();
        failstart = start;
        CK_BYTE_PTR wrapped_key;
        CK_ULONG wrapped_len;
        fail_counter = 0;
        while(1) {
            wrapped_key = NULL;
            wrapped_len = 0;
            rv = hsm_aes_encrypt(*session, *new_wrapping_key, decryption_key, decryption_key_length, &wrapped_key, &wrapped_len);

            if (rv == 7) {
                reset_connection(session);
                continue;
            } else if (rv == CKR_OK) {
                break;
            } 
            fail_counter += 1;
            sleep(1);
            failcont = get_time_in_seconds();
            time_counter_encryption_fail_time += failcont - failstart;
            failstart = failcont;
        }
        end = get_time_in_seconds();
        time_counter_encryption_wrap_dek += end - start;

        if (rv != CKR_OK) {
            return rc;
        }


        // Store new wrap in Redis datastore
        start = get_time_in_seconds();
        set(kv_key_wrap, wrapped_key, wrapped_len, conn);
        //close_redis (conn);
        end = get_time_in_seconds();
        time_counter_encryption_store_ciphertext += ((double) (end - start));
    }

    delete_old_k_master(session, old_key_handle);
    return 0;
}

int update_dek_key ( CK_SESSION_HANDLE_PTR session, 
   //                     memcached_st *memc, 
                        char* database_key, 
                        //unsigned int key_count, 
                        CK_OBJECT_HANDLE key_handle) {

    CK_RV rv;
    //generate_new_k_master(session, 16, new_wrapping_key);

    char* message;
    int message_length = 0;
    int attempt = 0;
    rv = download_and_decrypt (session, database_key, key_handle, &message, &message_length );
    while (rv != CKR_OK && attempt < 5) {
        printf("Download failed.\n");
        rv = download_and_decrypt (session, database_key, key_handle, &message, &message_length );
	attempt +=1;
    }

    if (rv != CKR_OK) {
        return -1;
    }

    rv = encrypt_and_upload(session, key_handle, database_key, message, message_length);
    free(message);

    //}
    return 0;
}

int update_master_and_dek_key ( CK_SESSION_HANDLE_PTR session, 
   //                     memcached_st *memc, 
                        char** database_keys, 
                        unsigned int key_count, 
                        CK_OBJECT_HANDLE old_key_handle, 
                        CK_OBJECT_HANDLE *new_wrapping_key) {

    start = get_time_in_seconds();
    CK_RV rv;
    generate_new_k_master(session, 16, new_wrapping_key);
    end = get_time_in_seconds();
    time_counter_keyupdate_createkey =  end - start;

    for (unsigned int i=0; i< key_count; i++) {
        char* key_id = database_keys[i];
        char* message;
        int message_length = 0;
        rv = download_and_decrypt (session, key_id, old_key_handle, &message, &message_length );
        if (rv != CKR_OK) {
            continue;
        }

        rv = encrypt_and_upload(session, *new_wrapping_key, key_id, message, message_length);
        free(message);
    }
    return 0;
}


int updatable_encrypt_and_upload(CK_SESSION_HANDLE_PTR session, 
		CK_OBJECT_HANDLE wrapping_key_handle,
		char* ciphertext_id, 
		char* message, 
		int message_length, 
		int total_re_encrypts) {


    // Setup Keys to Lookup Values on Redis Store
    char wrap_text[] = "wrap_";
    char header_text[] = "header_";
    char data_text[] = "data_";
    //char ctxt_version_number[] = "ctxt_version_";
    char root_key_version_number[] = "root_version_";

    char* kv_key_wrap = malloc(strlen(wrap_text) + strlen(ciphertext_id)+1);
    char* kv_key_header = malloc(strlen(header_text) + strlen(ciphertext_id)+1);
    char* kv_key_data = malloc(strlen(data_text) + strlen(ciphertext_id)+1);
    char* kv_key_rkvn = malloc(strlen(root_key_version_number) + strlen(ciphertext_id)+1);

    kv_key_wrap[0] = '\0';
    kv_key_header[0] = '\0';
    kv_key_data[0] = '\0';
    kv_key_rkvn[0] = '\0';

    strcat(kv_key_wrap, wrap_text);
    strcat(kv_key_wrap, ciphertext_id);
    strcat(kv_key_header, header_text);
    strcat(kv_key_header, ciphertext_id);
    strcat(kv_key_data, data_text);
    strcat(kv_key_data, ciphertext_id);
    strcat(kv_key_rkvn, root_key_version_number);
    strcat(kv_key_rkvn, ciphertext_id);

    // Encrypt Data using Updatable Encryption
    AE_key ae_key;
    AE_KeyGen( & ae_key, total_re_encrypts);

    ct_hat_data_en ciphertext_hat;
    int buffer_length = sizeof(AE_ctx_header) + message_length + total_re_encrypts * (2 * RHO + NU);
    uint8_t * ciphertext = (uint8_t * ) malloc(buffer_length);

    int ctx_length = AE_Encrypt(&ae_key, message, &ciphertext_hat, ciphertext, message_length);


    CK_RV rv;
    //int rc = EXIT_FAILURE;
    
    start = get_time_in_seconds();

    double failstart = get_time_in_seconds();
    double failcont;
    // Wrap the Decryption Key  
    CK_BYTE_PTR wrapped_key = NULL;
    CK_ULONG wrapped_len = 0;
    int fail_counter = 0;
    while(1) {
        wrapped_key = NULL;
        wrapped_len = 0;
        rv = hsm_aes_encrypt(*session, wrapping_key_handle, &ae_key, sizeof(AE_key), &wrapped_key, &wrapped_len);
        if (rv == CKR_OK) {
            break;
        }
        fail_counter += 1;
        sleep(1);
        failcont = get_time_in_seconds();
        time_counter_encryption_fail_time += failcont - failstart;
        failstart = failcont;
    }

    if (rv != CKR_OK) {
        printf("HSM Aes Encrypt Failed\n");
        free(wrapped_key);
        free(kv_key_wrap);
        free(kv_key_data);
        return rv;
    }
    double end = get_time_in_seconds();
    time_counter_encryption_wrap_dek += ((double) (end - start));

    // Convert key handle to a string and then into base64
    char key_handle_string[15];
    sprintf(key_handle_string, "%d", wrapping_key_handle);
    char* b64_key_handle = base64_enc((char*) key_handle_string, 15);
    

    /* Upload Header and Ciphertext (no wrap yet) */
    redisContext *conn = NULL;
    // set(kv_key_wrap, (char*)wrapped_key, wrapped_len, conn);
    // set(kv_key_header, &ciphertext_hat, sizeof (ct_hat_data_en), conn);
    // set(kv_key_data, ciphertext, buffer_length, conn);
    char* keys[4] = {kv_key_wrap, kv_key_header, kv_key_data, kv_key_rkvn};
    char* values[4] = {(char*)wrapped_key, &ciphertext_hat, ciphertext, b64_key_handle};
    char* value_sizes[4] = {wrapped_len, sizeof (ct_hat_data_en), buffer_length, 15};
    setall(4, keys, values, value_sizes, conn);


    //char* b64_ciphertext_hat = base64_enc((char*)&ciphertext_hat, sizeof(ct_hat_data_en));
    //printf("b64 ciphertext_hat from encrypt: \n");
    //printf("%s\n", b64_ciphertext_hat);

    //close_redis (conn);
    free (wrapped_key);
    free (kv_key_wrap);
    free (kv_key_data);
    free (kv_key_header); 
    free (ciphertext);
    return 0;
}



//  WORKING HERE
//  asdlfkads
int updatable_download_and_decrypt( CK_SESSION_HANDLE_PTR session, 
				    char* ciphertext_id, 
                                    char** retrieved_message,
                                    int* retrieved_message_length ) {
    // CK_OBJECT_HANDLE wrapping_key_handle will be deprecated


    // Setup Keys to Lookup Values on Redis Store
    char wrap_text[] = "wrap_";
    char header_text[] = "header_";
    char data_text[] = "data_";
    //char ctxt_version_number[] = "ctxt_version_";
    char root_key_version_number[] = "root_version_";

    char* kv_key_wrap = malloc(strlen(wrap_text) + strlen(ciphertext_id)+1);
    char* kv_key_header = malloc(strlen(header_text) + strlen(ciphertext_id)+1);
    char* kv_key_data = malloc(strlen(data_text) + strlen(ciphertext_id)+1);
    char* kv_key_rkvn = malloc(strlen(root_key_version_number) + strlen(ciphertext_id)+1);

    kv_key_wrap[0] = '\0';
    kv_key_header[0] = '\0';
    kv_key_data[0] = '\0';
    kv_key_rkvn[0] = '\0';

    strcat(kv_key_wrap, wrap_text);
    strcat(kv_key_wrap, ciphertext_id);
    strcat(kv_key_header, header_text);
    strcat(kv_key_header, ciphertext_id);
    strcat(kv_key_data, data_text);
    strcat(kv_key_data, ciphertext_id);
    strcat(kv_key_rkvn, root_key_version_number);
    strcat(kv_key_rkvn, ciphertext_id);

    //setup redis connection
    redisContext *conn = NULL;
    //int return_val = init_redis (&conn);

    // Retrieve Wrap, Header, and Ciphertext. 
    // int wrap_length = 0;
    // char* ae_key_wrap = get(kv_key_wrap, conn, &wrap_length);        
    // int header_length = 0;
    // ct_hat_data_en* ciphertext_hat = (ct_hat_data_en*)get(kv_key_header, conn, &header_length);
    // int data_length = 0;
    // uint8_t * ciphertext = (uint8_t *) get(kv_key_data, conn, &data_length);


    char* kv_key[4];
    kv_key[0] = kv_key_wrap;
    kv_key[1] = kv_key_header;
    kv_key[2] = kv_key_data;
    kv_key[3] = kv_key_rkvn;
    int* sizes;
    char** downloaded_values = getall(4, kv_key, &sizes, conn);
    int wrap_length = sizes[0];
    char* ae_key_wrap = downloaded_values[0];
    int header_length = sizes[1];
    ct_hat_data_en* ciphertext_hat = (ct_hat_data_en*)downloaded_values[1];
    int data_length = sizes[2];
    uint8_t * ciphertext = (uint8_t *)downloaded_values[2];

    CK_OBJECT_HANDLE wrapping_key_handle = atoi(downloaded_values[3]);

    
    free(downloaded_values);
    free(sizes);

    if (data_length == 0 || header_length == 0) {
        return -1; // key not in redis server
    }

    CK_RV rv;
    double start = get_time_in_seconds();
    // Decrypt wrap with master key
    CK_BYTE_PTR decryption_key = NULL;
    CK_ULONG decryption_key_length = 0;

    double failstart = get_time_in_seconds();
    double failcont;

    int fail_counter = 0;

    while(1) {

        //printf("Wrapping Key Handle: %d\n", wrapping_key_handle);
        //printf("Wrapped Key: %s\n", ae_key_wrap);
        //printf("Wrapped Key Len: %d\n", wrap_length);
        rv = hsm_aes_decrypt(*session, wrapping_key_handle, ae_key_wrap, wrap_length, &decryption_key, &decryption_key_length);
        if (rv == 7) {
            reset_connection(session);
            continue;
        } else if (rv == CKR_OK) {
            break;
        } else {
            printf("HSM_AES_DECRYPT ERROR\n");
        }
        printf("Wrap Length: %d\n", wrap_length);
        printf("Decryption Key Length: %d\n", decryption_key_length);
        fail_counter += 1;
        sleep(1);
        failcont = get_time_in_seconds();
        time_counter_decryption_fail_time += failcont - failstart;
        failstart = failcont;
    }
    end = get_time_in_seconds();
    time_counter_decryption_decrypt_wrap += end - start;


    AE_key* ae_key = (AE_key*)decryption_key;

    // Close Redis Connection
    //close_redis (conn);


    // AE_Decrypt
    uint8_t * decrypted_message = (uint8_t * ) malloc(data_length);
    //bzero(decrypted_message, data_length);
    AE_Decrypt(ae_key, ciphertext_hat, ciphertext, decrypted_message, data_length - 32);


    // this retrieved_message_length is currently incorrect
    *retrieved_message_length = strlen(decrypted_message) + 1; // +1 because null terminator
    *retrieved_message = malloc( *retrieved_message_length);
    memset(*retrieved_message, 0, *retrieved_message_length);
    strncpy(*retrieved_message, decrypted_message, *retrieved_message_length);
    printf("retrieved_message_length: %d\n", *retrieved_message_length);


    free(decrypted_message);
    free(ae_key_wrap);
    free(ciphertext_hat);
    free(ciphertext);
    free(kv_key_wrap);
    free(kv_key_header);
    free(kv_key_data);
    free(kv_key_rkvn);
    free(decryption_key);

    return 0;
}

int updatable_update_dek_and_ciphertext(CK_SESSION_HANDLE_PTR session,  char* ciphertext_id, int total_re_encrypts)
{
    // Setup Keys to Lookup Values on Redis Store
    char wrap_text[] = "wrap_";
    char header_text[] = "header_";
    char data_text[] = "data_";
    //char ctxt_version_number[] = "ctxt_version_";
    char root_key_version_number[] = "root_version_";

    char* kv_key_wrap = malloc(strlen(wrap_text) + strlen(ciphertext_id)+1);
    char* kv_key_header = malloc(strlen(header_text) + strlen(ciphertext_id)+1);
    char* kv_key_data = malloc(strlen(data_text) + strlen(ciphertext_id)+1);
    char* kv_key_rkvn = malloc(strlen(root_key_version_number) + strlen(ciphertext_id)+1);

    kv_key_wrap[0] = '\0';
    kv_key_header[0] = '\0';
    kv_key_data[0] = '\0';
    kv_key_rkvn[0] = '\0';

    strcat(kv_key_wrap, wrap_text);
    strcat(kv_key_wrap, ciphertext_id);
    strcat(kv_key_header, header_text);
    strcat(kv_key_header, ciphertext_id);
    strcat(kv_key_data, data_text);
    strcat(kv_key_data, ciphertext_id);
    strcat(kv_key_rkvn, root_key_version_number);
    strcat(kv_key_rkvn, ciphertext_id);

    //setup redis connection
    redisContext *conn = NULL;
    // int return_val = init_redis (&conn);

    // Retrieve Wrap, Header, and Ciphertext. 
    // int wrap_length = 0;
    // char* ae_key_wrap = get(kv_key_wrap, conn, &wrap_length);     
    // int header_length = 0;
    // ct_hat_data_en* ciphertext_hat = (ct_hat_data_en*)get(kv_key_header, conn, &header_length);

    char** kv_keys[2];
    kv_keys[0] = kv_key_wrap;
    kv_keys[1] = kv_key_header;
    kv_keys[2] = kv_key_rkvn;
    int* sizes;
    char** downloaded_values = getall(3, kv_keys, &sizes, conn);
    int wrap_length = sizes[0];
    char* ae_key_wrap = downloaded_values[0];
    int header_length = sizes[1];
    ct_hat_data_en* ciphertext_hat = (ct_hat_data_en*)downloaded_values[1];

    CK_OBJECT_HANDLE wrapping_key_handle = atoi(downloaded_values[2]);

    free (downloaded_values);
    free (sizes);
        
    if (wrap_length <= 0 || header_length <= 0) {
        printf ("returning early\n");
        return -1;
    }


    CK_RV rv;

    double start = get_time_in_seconds();
    // Decrypt wrap with master key
    CK_BYTE_PTR decryption_key = NULL;
    CK_ULONG decryption_key_length = 0;

    double failstart = get_time_in_seconds();
    double failcont;

    int fail_counter = 0;
    while(1) {
        rv = hsm_aes_decrypt(*session, wrapping_key_handle, ae_key_wrap, wrap_length, &decryption_key, &decryption_key_length);
        if (rv == 7) {
            reset_connection(session);
            continue;
        } else if (rv == CKR_OK) {
            break;
        } else {
            printf("HSM_AES_DECRYPT ERROR\n");
	    return -1;
        }
        fail_counter += 1;
        sleep(1);
        failcont = get_time_in_seconds();
        time_counter_decryption_fail_time += failcont - failstart;
        failstart = failcont;
    }

    end = get_time_in_seconds();
    time_counter_decryption_decrypt_wrap += end - start;

    AE_key* ae_key = (AE_key*)decryption_key;

    char* b64_ciphertext_hat = base64_enc((char*)ciphertext_hat, sizeof(ct_hat_data_en));

    AE_key ae_key2;
    AE_KeyGen( & ae_key2, total_re_encrypts);

    
    start = get_time_in_seconds();
    failstart = get_time_in_seconds();
    failcont;
    // Wrap the Decryption Key  
    CK_BYTE_PTR wrapped_key2 = NULL;
    CK_ULONG wrapped_len2 = 0;
    fail_counter = 0;
    while(1) {
        wrapped_key2 = NULL;
        wrapped_len2 = 0;
        rv = hsm_aes_encrypt(*session, wrapping_key_handle, &ae_key2, sizeof(AE_key), &wrapped_key2, &wrapped_len2);
        if (rv == CKR_OK) {
            break;
        }
        fail_counter += 1;
        sleep(1);
        failcont = get_time_in_seconds();
        time_counter_encryption_fail_time += failcont - failstart;
        failstart = failcont;
    }

    if (rv != CKR_OK) {
        printf("HSM Aes Encrypt Failed\n");
        free(wrapped_key2);
        free(kv_key_wrap);
        free(kv_key_data);
        return rv;
    }
    end = get_time_in_seconds();
    time_counter_encryption_wrap_dek += ((double) (end - start));

    delta_token_data delta;
    AE_ReKeyGen( ae_key, & ae_key2, ciphertext_hat, & delta);


    char* b64_delta = base64_enc(&delta, sizeof(delta_token_data));
    char* b64_aekey_wrap2 = base64_enc((char*)wrapped_key2, wrapped_len2);
    //redisReply *reply = redisCommand(conn,"GEOWEIYEE %s %s %s %s %s", kv_key_wrap, kv_key_header, kv_key_data, b64_aekey_wrap2, b64_delta);
    
    //printf("strlen stfu\n");

    int strsize = strlen ("UP_UPDATE ") + strlen (kv_key_wrap) + 1 + strlen (kv_key_header) + 1 + strlen (kv_key_data) + 1 + strlen (b64_aekey_wrap2) + 1 + strlen (b64_delta) + 1;
    char* sendbuf = (char*)malloc (strsize * sizeof (char));
    bzero (sendbuf, strsize);
    sprintf (sendbuf, "UP_UPDATE %s %s %s %s %s", kv_key_wrap, kv_key_header, kv_key_data, b64_aekey_wrap2, b64_delta);

    //printf("Hello\n");
    init_redis (&conn);
    //printf("MID\n");
    int sockfd = get_sockfd();
    //printf ("Before send\n");
    sendall (sockfd, sendbuf, strsize);
    bzero(sendbuf, strsize);
    //printf ("Before recv\n");
    recvall (sockfd, sendbuf, strsize);  

    close_redis (conn);


    free (kv_key_wrap);
    free (kv_key_header);
    free (kv_key_data); 
    free (sendbuf);
    free (b64_delta);
    free (b64_aekey_wrap2);
    free (b64_ciphertext_hat);
    free (ae_key_wrap);
    free (ciphertext_hat);
    free (wrapped_key2);
    free (decryption_key);
}

