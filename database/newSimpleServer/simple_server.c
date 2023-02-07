#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include "aes_ctr.h"
#include "aes_gcm.h"
#include "AE_Nested_AES.h"
#include <time.h>
#include "hiredis.h"

#define BUFFMAX 16000000 // = 10 MB
//#define MAX 20000
#define PORT 5000
#define ARRAY_SIZE 20
#define SA struct sockaddr

char current_root_key[20];
pthread_mutex_t current_root_key_lock;


char* savebuf[BUFFMAX];
//char savebuf [BUFFMAX];
int multi_thread_on = 2;
pthread_mutex_t anti_rc_lock;		// anti race condition lock. Makes sure Get and Set occur atomically. Update will lock, get, unlock, update ciphertext, lock, set, unlock. 
pthread_mutex_t single_thread_mutex;
long long used_mutex_count = 0;

double get_time_in_seconds() {
    struct timeval  tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec) + (tv.tv_usec) / 1000000.0 ;
}


int init_redis (redisContext **conn) {
    
    const int port = 6379;
    char* ip_addr = "127.0.0.1";
    //const char *password = argv[3];
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    *conn = redisConnectWithTimeout(ip_addr, port, timeout);
    if (*conn == NULL || (*conn)->err) {

        if (conn) {
            printf("Connection error: %s ; Exiting...\n", (*conn)->errstr);
            redisFree(*conn);
        } else {
            printf("Connection error: can't allocate redis context; Exiting...\n");
        }
        exit(1);
    }
    return 0;
}

// set for redis
int redisSet(char *key, char *value, int value_length, redisContext *conn) {
    redisReply *reply;

    reply = redisCommand(conn,"SET %s %s", key, value);
    freeReplyObject(reply);
    return 0;
}

// get for redis
char* redisGet(char *key, redisContext *conn, int *length) {
    /* Get */
    redisReply *reply;

    reply = redisCommand(conn,"GET %s", key);
    *length = reply->len;
    char* res = reply->str;

    //freeReplyObject(reply);
    return res;   
}

int close_redis (redisContext *conn) {
	redisFree(conn);
	return 0;
}



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

int sendall (int sockfd, void* sendbuf, int sendsize) {
	int total_bytes = 0;
	int nbytes = 0;

	void* startbuf = sendbuf;

	int max_send = 50000;
	int current_send = sendsize; 
	if (current_send > max_send) {
		current_send = max_send;
	}

	int i = 0;
	//printf ("Before send\n");
	while (sendsize > 0 && (nbytes = send(sockfd, startbuf, current_send, 0)) > 0 ) {
		startbuf += nbytes;
		sendsize -= nbytes;
		total_bytes += nbytes;

		current_send = sendsize; 
		if (current_send > max_send) {
			current_send = max_send;
		}

		//printf("iteration %d\n", i);
		//printf("nbytes: %d\n", nbytes);
		i += 1;
	}

	//printf ("total_bytes: %d\n", total_bytes);
	//printf ("nbytes: %d\n", nbytes);
	//printf ("sendsize: %d\n", sendsize);
	//printf ("Sent: %s\n", (char*) sendbuf);
	return total_bytes;
}


char *base64_enc(const char *str, int len)
{
	BIO *bio, *b64;
	BUF_MEM *bptr;
	char *buf;
	int ret;


	b64 = BIO_new(BIO_f_base64());
	bio = BIO_new(BIO_s_mem());
	bio = BIO_push(b64, bio);
	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);


	ret = BIO_write(bio, str, len);
	if (ret <= 0) {
		buf = NULL;
		goto err;
	}
	BIO_flush(b64);
	BIO_get_mem_ptr(b64, &bptr);

	buf = (char*)malloc(bptr->length + 1);
	if (buf) {
		memcpy(buf, bptr->data, bptr->length+1);
		buf[bptr->length] = 0;
	}

err:
	BIO_free_all(b64);

	return buf;
}

char *base64_dec(char *str, int len, int *result_len)
{
	BIO *bio, *b64;
	char *buf;
	int ret;

	if (!(buf = (char*)malloc(len + 1)))
		return NULL;

	memset(buf, 0, len + 1);

	b64 = BIO_new(BIO_f_base64());
	bio = BIO_new_mem_buf(str, len);
	bio = BIO_push(b64, bio);
	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

	ret = BIO_read(bio, buf, len);
	if (ret <= 0) {
		free(buf);
		buf = NULL;
	}   
	BIO_free_all(bio);


	if (result_len)
		*result_len = ret;


	return buf;
}

char** split_by_underscore (char *str, int* count) {

	//printf("%s\n", str);

	int length = strlen(str);
	char* token;

	char* copy = malloc (length + 1);
	memset (copy, 0, length + 1);
	memcpy (copy, str, length);
	char* rest = copy;
	(*count) = 0;
	while ((token = strtok_r(rest, "_", &rest))) {
		(*count)++;
	}

	char** string_arr = malloc(*count * sizeof(char*));
	memset(copy, 0, length+1);
	strncpy(copy, str, length);
	rest = copy;
	int index = 0;
	while ((token = strtok_r(rest, "_", &rest))) {
		string_arr[index] = token;
		index ++;
	}

	return string_arr;
}


char** split_by_space (char *str, int* count) {

	//printf("%s\n", str);

	int length = strlen(str);
	char* token;

	char* copy = malloc (length + 1);
	memset (copy, 0, length + 1);
	memcpy (copy, str, length);
	char* rest = copy;
	(*count) = 0;
	while ((token = strtok_r(rest, " ", &rest))) {
		(*count)++;
	}

	char** string_arr = malloc(*count * sizeof(char*));
	memset(copy, 0, length+1);
	strncpy(copy, str, length);
	rest = copy;
	int index = 0;
	while ((token = strtok_r(rest, " ", &rest))) {
		string_arr[index] = token;
		index ++;
	}

	return string_arr;
}


/// OLD Get Value
// int get_value(char* key, uint8_t** value, int* database_size, char** key_array, char** value_array) {
// 	if (multi_thread_on == 1) {
// 		pthread_mutex_lock(&single_thread_mutex);
// 	}

// 	for (int i=0; i<*database_size; i++) {
// 		//printf("Index: %d\n", i);
// 		//printf("keylen: %d\n", strlen(key));
// 		//printf("keyarrlen: %d\n",strlen(key_array[i]));
// 		//printf("key: %s\n", key);
// 		//printf("key_array[i]: %s\n", key_array[i]);
// 		if(strlen(key) == strlen(key_array[i]) && strncmp(key, key_array[i], strlen(key)) == 0) {
// 			*value = value_array[i];

// 			if (multi_thread_on == 1) {
// 				pthread_mutex_unlock(&single_thread_mutex);
// 			}
// 			return strlen(*value);
// 		}
// 	}

// 	if (multi_thread_on == 1) {
// 		pthread_mutex_unlock(&single_thread_mutex);
// 	}

// 	return 0;
// }




int get_value(char* key, uint8_t** value, redisContext *conn) {

	int length; 
	*value = redisGet(key, conn, &length);

	if (length == 0) {
		*value = NULL;
	}



	return length;
}


///// OLD GET ALL
// int getall_values (int numkeys, char** keys, uint8_t** values, int* database_size, char** key_array, char** value_array) {
// 	uint8_t** arr_values = malloc (numkeys * sizeof (uint8_t *));
// 	int * sizes = malloc (numkeys * sizeof (int));
// 	int sizesum = 0;
// 	for (int i = 0; i < numkeys; i++) {
// 		sizes[i] = get_value(keys[i], &arr_values[i], database_size, key_array, value_array);	
// 		sizesum += sizes[i] + 1;
// 	}
	
// 	*values = malloc (sizesum);
// 	*values[0] = 0;
// 	for (int i = 0; i < numkeys; i++) {
// 		strncat ((char*) (*values), (char*) (arr_values[i]), sizes[i]);
// 		if (i < numkeys - 1) {
// 			strncat ((char*) (*values), " ", 1);
// 		}
// 	}
// 	(*values)[sizesum-1] = 0;

// 	free(sizes);
// 	free(arr_values);
// 	return sizesum;

// }


int getall_values (int numkeys, char** keys, uint8_t** values) {

	// Init some starting datastructures. N
	uint8_t** arr_values = malloc (numkeys * sizeof (uint8_t *));
	int * sizes = malloc (numkeys * sizeof (int));
	int sizesum = 0;

	

	// Init Redis Connection
	redisContext *conn = NULL;
	int return_val = init_redis(&conn);

	// acquire lock before accessing data on redis
	pthread_mutex_lock(&anti_rc_lock);

	// Loop through numkeys and get this many items
	for (int i = 0; i < numkeys; i++) {
		sizes[i] = get_value(keys[i], &arr_values[i], conn);
		printf("Key: %s\n", keys[i]);
		printf("sizes[i]: %d\n", sizes[i]);

		sizesum += sizes[i] + 1;
	}

	// release lock
	pthread_mutex_unlock(&anti_rc_lock);


	// Now we have the size, we can malloc values and return the value into there. 
	*values = malloc(sizesum);
	*values[0] = 0;
	for (int i = 0; i < numkeys; i++) {
		strncat ((char*) (*values), (char*) (arr_values[i]), sizes[i]);
		if (i < numkeys - 1) {
			strncat ((char*) (*values), " ", 1);
		}
	}
	(*values)[sizesum-1] = 0;

	// free stuff we don't need anymore
	free(sizes);
	free(arr_values);
	close_redis(conn);

	return sizesum;

}



int set_value(char* key, char* value, redisContext* conn) {

	redisSet(key, value, strlen(value), conn);
	return 1;
}





int setall_values(int numkeys, char** key_values) {
	printf("Starting SEtall\n");

	//printf("NumKeys: %d\n", numkeys);

	char* ctxt_root_key_version = "default";
	char* key_suffix;


	// Check root key version is the same
	// determine which index the root key version is from key_values. Then I'll 
	int root_key_version_matches = 0;
	for (int i = 0; i < (int)(numkeys / 2); i++) {
		char* key_name = key_values[i*2];

		// check if root key version exists
		printf("Checking root key version exists\n");
		if (strlen(key_name) >= 13 && strncmp(key_name, "root_version_", 13) == 0) {
			printf("Guess we're in\n");
			printf("root_key_version: %s\n", key_name);

			// acquire lock for accessing root key version
			pthread_mutex_lock(&current_root_key_lock);

			// now check if root key matches
			ctxt_root_key_version = key_values[i*2  + 1];
			printf("i: %d\n", i);
			printf("i*2+1: %d\n", (i*2 + 1));
			if (strlen(ctxt_root_key_version) == strlen(current_root_key) && strncmp(ctxt_root_key_version, current_root_key, strlen(current_root_key)) == 0) {
				printf("we made sure ctxt_root_key is equal to current_root key?\n");
				root_key_version_matches = 1;
				int num_of_splits;
				printf("SPLITGIN THIS: %s\n", key_values[i*2]);
				char** split_arr = split_by_underscore(key_values[i*2], &num_of_splits);
				printf("num_of_splits: %d\n", num_of_splits);
				printf("split: %s\n", split_arr[0]);
				printf("split: %s\n", split_arr[1]);
				printf("split: %s\n", split_arr[2]);
				key_suffix = split_arr[num_of_splits-1];
				printf("key_suffix: %s\n", key_suffix);
			}

			// release lock for accessing root key version
			pthread_mutex_unlock(&current_root_key_lock);

			// break either way
			break;
		}
	}


	if (root_key_version_matches == 1) {
		printf("We did match on root key so we will do setall stuff\n");
		redisContext *conn = NULL;
		int return_val = init_redis(&conn);

		char ctxt_version_number[] = "ctxt_version_";
		printf("Do strcat stuff here\n");
		printf("ctxt_version_number: %s\n", ctxt_version_number);
		printf("strlen of ctxt_version_number: %d\n", strlen(ctxt_version_number));
		printf("key_suffix: %s\n", key_suffix);
		printf("strlen of key_suffix: %d\n", strlen(key_suffix));
		char* kv_ctxt_version = malloc(strlen(ctxt_version_number) + strlen(key_suffix)+1);
		kv_ctxt_version[0] = '\0';
    	strcat(kv_ctxt_version, ctxt_version_number);
    	strcat(kv_ctxt_version, key_suffix);
    	printf("kv_ctxt_version key name: %s\n", kv_ctxt_version);


		// acquire lock before setting data on redis
		pthread_mutex_lock(&anti_rc_lock);

		printf("get ctxt version value\n");
		// get ctxt version value
		char* ctxt_version_value;
		get_value(kv_ctxt_version, &ctxt_version_value, conn);
		printf("Done: value is: %s\n", ctxt_version_value);

		// convert to an int, increment, then convert back to string
		printf("Convert stuff\n");
		int ctxt_version_int = atoi(ctxt_version_value);
		ctxt_version_int += 1;
	    char new_ctxt_version_value[10];
	    sprintf(new_ctxt_version_value, "%d", ctxt_version_int);
	    printf("old int version: %d\n", ctxt_version_int);
	    printf("new ctxt version value; %s\n", new_ctxt_version_value);


		// set ctxt version number again
		printf("SET VALUE\n");
		set_value(kv_ctxt_version, new_ctxt_version_value, conn);
		printf("DONE SET VALUE\n");

		// Set all the values here
		printf("SET REST OF VALUE EHRE\n");
		for (int i = 0; i < (int)(numkeys / 2); i++) {
			char* key = key_values[i*2];
			char* value = key_values[i*2 + 1];

			//printf("key: %s\n", key);
			//printf("value: %s\n", value);
			set_value(key, value, conn);
		}
		printf("DONE WITH REST\n");


		// release lock and close connection
		pthread_mutex_unlock(&anti_rc_lock);
		close_redis(conn);


		return 1;
	} else {
		printf("Root Key did not match\n");
		printf("Expected  %s\n as the root key\n", current_root_key);
		printf("Given     %s\n as the root key\n", ctxt_root_key_version);

		return 0;
	}


}

// CD Updatable 
// param 1 : write wrap id
// param 2 : read / write ciphertext header id
// param 3 : read / write ciphertext data id
// param 4 : new wrap
// param 5 : delta


int up_update(char* wrap_key_id, char* ciphertext_header_id, char* ciphertext_data_id, char* ciphertext_version_id, char* new_wrap, char* delta, 
		int* database_size, char** key_array, char** value_array) {

	redisContext *conn = NULL;
	int return_val = init_redis(&conn);


	//////////////////////GET VALUES FROM DB///////////////////////////////

	// acquire lock before accessing redis resources
	pthread_mutex_lock(&anti_rc_lock);

	// get ctxt version value
	char* ctxt_version_value;
	get_value(ciphertext_version_id, &ctxt_version_value, conn);

	

	// Read old ciphertext header for update.
	char* ciphertext_header;
	int res = 0;
	res = get_value(ciphertext_header_id, &ciphertext_header, conn);
	if (res == 0) {
		//printf("-------FIRST--------\ndbsize: %d\norig wrap_key_id: %s\norig wrap_key_id: %s\norig wrap_key_id: %s\nwrap_key_id: %s\nciphertext_header_id: %s\nct_data_id: %s\nres is 0. first\n", *database_size, key_array[0], key_array[1], key_array[2], wrap_key_id, ciphertext_header_id, ciphertext_data_id);
		// printf("orig wrap_key_id: %s\n", key_array[1]);
		// printf("orig wrap_key_id: %s\n", key_array[2]);
		// printf("wrap_key_id: %s\n", wrap_key_id);
		// printf("res is 0. first\n");
	}

	// Read old ciphertext data for update.
	char* ciphertext_data;
	res = get_value(ciphertext_data_id, &ciphertext_data, conn);

	// Release Lock after accessing resources
	pthread_mutex_unlock(&anti_rc_lock);



	////////////////////////////UPDATABLE ENCRYPTION OCCURS HERE////////////////////////////

	if (res == 0) {
		//printf("-------SECOND--------\ndbsize: %d\norig wrap_key_id: %s\norig wrap_key_id: %s\norig wrap_key_id: %s\nwrap_key_id: %s\nciphertext_header_id: %s\nct_data_id: %s\nres is 0. first\n", *database_size, key_array[0], key_array[1], key_array[2], wrap_key_id, ciphertext_header_id, ciphertext_data_id);
	}

	//printf("ciphertext_data: %s\n", ciphertext_data);
	// printf("before print strlen\n");
	// printf("strlen: %d\n", strlen(ciphertext_data));

	int header_len = 0;
	char* header_bytes = base64_dec(ciphertext_header, strlen(ciphertext_header), &header_len);
	int data_len = 0;
	char* data_bytes = base64_dec(ciphertext_data, strlen(ciphertext_data), &data_len);


	// base64 decode delta
	int delta_len = 0;
	char* delta_bytes = base64_dec(delta, strlen(delta), &delta_len);

	ct_hat_data_en ciphertext_hat2;
	uint8_t * ciphertext2 = (uint8_t *) malloc (data_len);

	// Rencrypt
	AE_ReEncrypt((delta_token_data*) delta_bytes, (ct_hat_data_en*) header_bytes, (uint8_t*) data_bytes, &ciphertext_hat2, ciphertext2,  data_len - 32);

	// base64 encode wrap and data
	char* new_header = (char*)base64_enc((char*)&ciphertext_hat2, header_len);
	char* new_ciphertext = (char*)base64_enc((char*)ciphertext2, data_len);

	printf("INSIDE UPDATE\n");
	printf("datalen: %d\n", data_len);
	printf("new  updated ciphertext size: %d\n", strlen(new_ciphertext));


	////////////////////////SET UPDATE HERE///////////////////////////////

	pthread_mutex_lock(&anti_rc_lock);

	char* ctxt_version_value_after;
	get_value(ciphertext_version_id, &ctxt_version_value_after, conn);
	if (strlen(ctxt_version_value_after) == strlen(ctxt_version_value) && strncmp(ctxt_version_value_after, ctxt_version_value, strlen(ctxt_version_value)) == 0) {
		printf("Doing stuff in here\n");
		// convert to an int, increment, then convert back to string
		int ctxt_version_int = atoi(ctxt_version_value_after);
		ctxt_version_int += 1;
	    char new_ctxt_version_value[10];
	    sprintf(new_ctxt_version_value, "%d", ctxt_version_int);
	
	    // Overwrite wrap with new wrap
		set_value(wrap_key_id, new_wrap, conn);

		// Overwrite header with new header
		set_value(ciphertext_header_id, new_header, conn);

		// Overwrite old ciphertext with new ciphertext.
		set_value(ciphertext_data_id, new_ciphertext, conn);

		// Overwrite new ciphertext version number
		set_value(ciphertext_version_id, new_ctxt_version_value, conn);
	} else {
		printf("Ciphertext did no match sadly. Retry\n");

		printf("ctxt version key id: %s\n", ciphertext_version_id);
		printf("First: %s\n", ctxt_version_value_after);
		printf("Secon: %s\n", ctxt_version_value);
		printf("Strlen(First): %d\n", strlen(ctxt_version_value_after));
		printf("Strlen(Secon): %d\n", strlen(ctxt_version_value));
		pthread_mutex_unlock(&anti_rc_lock);
		return 1;

	}

	pthread_mutex_unlock(&anti_rc_lock);



	// // New Frees
	free(header_bytes);
	free(data_bytes);
	free(delta_bytes);
	free(ciphertext2);
	free(new_header);
	free(new_ciphertext);

	close_redis(conn);

	return 0;
}

int myThreadFun(int* t_num) {
	int thread_num = *(t_num);

	int database_size = 0;
	char* key_array[ARRAY_SIZE];
	char* value_array[ARRAY_SIZE]; 

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
	servaddr.sin_port = htons(PORT + thread_num);

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
		printf("Listen failed on port %d...\n", PORT + thread_num);
		exit(0);
	}
	else {
		printf("Server listening on port %d..\n", PORT + thread_num);
	}
	len = sizeof(cli);

	char* buff = (char*)malloc (BUFFMAX);
	//char buff[MAX];

	// ----------- Where the loop starts ------------

	int prevconnection = -1;
	double start, end;



	for(;;) {


		start = get_time_in_seconds();
		connfd = accept(sockfd, (SA*)&cli, &len);
		if (connfd < 0) {
			printf("server accept failed...\n");
			//exit(0);
		}
		else {
			//printf("server accept the client...\n");
		}
	        end = get_time_in_seconds();
	        double connect_to_server = (end - start);

	    // LOCK THREAD. Then increment a tracker count
		if (multi_thread_on == 0) {
			pthread_mutex_lock(&single_thread_mutex);
			used_mutex_count += 1;
		}

		// read the message from client  
		//bzero(buff, BUFFMAX);
		recvall(connfd, buff, BUFFMAX);

		//printf("GOT MESSAGE\n");

		// parse into commands
		//printf("Buffer Command: %s\n", buff);
		int number_of_splits;
		char** command_splits = split_by_space(buff, &number_of_splits);
		char* command = command_splits[0];


		// Run that command (set,get,updatable_update)
		uint8_t* result = "0";
		int result_size = 1;

		printf("Command: %s\n", command);
		printf("Number of Splits: %d\n", number_of_splits);

		//printf("len of command: %d\n", strlen(command));
		if (strlen(command) == 3 && strncmp(command, "GET", 3) == 0) {
			//printf("GET\n");
			char* key = command_splits[1];

			redisContext *conn = NULL;
			int return_val = init_redis(&conn);

			result_size = get_value(key, &result, conn);

			close_redis(conn);
		}
	        
		else if (strlen(command) == 6 && strncmp(command, "GETALL", 6) == 0) {
			//printf("GETALL\n");
			// GETALL key1 key2...
			// Response: 
			// value1 value2 value3 ...
			int num = number_of_splits - 1;
   			result_size = getall_values (num, &command_splits[1], &result);
		}
		else if (strlen(command) == 3 && strncmp(command, "SET", 3) == 0) {
			//printf("Received set\n");
			char* key = command_splits[1];
			char* value = command_splits[2];

			//printf("KEY: %s\n", key);

			redisContext *conn = NULL;
			int return_val = init_redis(&conn);

			result_size = set_value(key, value, conn);

			close_redis(conn);
		} 

		else if (strlen(command) == 6 && strncmp(command, "SETALL", 6) == 0) {
			// GETALL key1 key2...
			// Response: 
			// value1 value2 value3 ...
			int num = number_of_splits - 1;
   			result_size = setall_values (num, &command_splits[1]);

   			//printf("Received setALL\n");
		}
		//else if (strlen(command) == 5 && strncmp(command, "CLOSE", 5) == 0) {
		//free(connfd);
		//return 0;
		//} 
		else if (strlen(command) == 9 && strncmp(command, "UP_UPDATE", 9) == 0) {
			//printf("received up_update\n");
			// param 1 : write wrap id
			// param 2 : read / write ciphertext header id
			// param 3 : read / write ciphertext data id
			// param 4 : ctxt version id
			// param 5 : new wrap
			// param 6 : delta
			char* wrap_id = command_splits[1];
			char* header_id = command_splits[2];
			char* data_id = command_splits[3];
			char* ctxt_ver = command_splits[4];
			char* new_wrap = command_splits[5];
			char* delta = command_splits[6];
			//printf("split_sizes upt: %d, %d, %d, %d, %d, %d", strlen(wrap_id), strlen(header_id), strlen(data_id), strlen(ctxt_ver), strlen(new_wrap), strlen(delta));

			result_size = up_update(wrap_id, header_id, data_id, ctxt_ver, new_wrap, delta, &database_size, key_array, value_array);
		// } else if (strlen(command) == 6 && strncmp(command, "KILLME", 6) == 0) {
		// 	close(connfd);
		} else if (strlen(command) == 8 && strncmp (command, "SAVEDATA", 8) == 0) {
			printf("Got Save data\n");
			char* data = command_splits[1];
			bzero(savebuf, BUFFMAX);
			memcpy (savebuf, data, strlen (data));
			printf("Completed save data\n");
		} // Also have to save ctxt version numbers and root key versions
		else if (strlen(command) == 8 && strncmp (command, "POPULATE", 8) == 0) {
			int num_keys = number_of_splits-1;				
			printf("Got populate data\n");
			
			redisContext *conn = NULL;
			int return_val = init_redis(&conn);
			for (int i = 0; i < num_keys; i++) {
				if (i % 1000 == 0) {
					printf("i: %d\n", i);
				}
				char* kvkey = command_splits[1+i];
				result_size = set_value(kvkey, savebuf, conn);
			}			
			printf("Completed populate data\n");
			close_redis(conn);
		} else if (strlen(command) == 7 && strncmp (command, "ROOTKEY", 7) == 0) {			
			printf("Updating Root Key\n");
			
			pthread_mutex_lock(&current_root_key_lock);
			printf("SIZEOF CURRENT ROOTKEY: %d\n", strlen(command_splits[1]));
			bzero(current_root_key, strlen(command_splits[1]));
			memcpy(current_root_key, command_splits[1], strlen(command_splits[1]));
			printf("Updated Root Key was %s\n", current_root_key);
			pthread_mutex_unlock(&current_root_key_lock);
			printf("Done updating root key\n");
		}
		else {
			printf("Dont recognize command\n");
		   	printf ("Received %s\n", command_splits[0]);
		   	if (multi_thread_on == 0) {
		   		pthread_mutex_unlock(&single_thread_mutex);
		   	}
			continue;
		}

	   	
		// write result from that command to connfd
		//printf("result: %s\n", result); 
		//bzero(buff, BUFFMAX);
		memcpy(buff, result, result_size);
		buff[result_size+1] = 0;
		//printf("buff: %s\n", buff);
		//printf("sizeofbuff: %d\n", sizeof(buff));
		//printf("strlenbuff: %d\n", strlen(buff));
		sendall(connfd, buff, strlen(buff)+1);
		buff[0] = 0;		
		//bzero(buff, BUFFMAX);
		if (prevconnection != -1) {
			close (prevconnection);
		}
		prevconnection = connfd;


		if (strlen(command) == 6 && strncmp(command, "GETALL", 6) == 0) {
			free(result);
		}
		free(command_splits[0]);
		free(command_splits);

		
		if (multi_thread_on == 0) {
	   		pthread_mutex_unlock(&single_thread_mutex);
	   	}

		//close (connfd);
		
	}
}


// Driver function
int main()
{
	int num_threads = 5;


	for (int i = 0; i < num_threads; i++) {

		pthread_t thread_id;
		int* thread_num = malloc(sizeof(int));
		*thread_num = i;
		pthread_create(&thread_id, NULL, myThreadFun, (void*) thread_num);
	}

	// Accept the data packet from client and verification
	for(;;) {
		sleep (10);
	}
}


// int main()
// {

// 	redisContext *conn = NULL;
// 	int return_val = init_redis(&conn);

// 	set_value("bbbbb", "What is going on?", conn);


// 	char* value;
// 	int length = get_value("bbbbb", &value, conn);
// 	printf("%s\n", value);

// 	close_redis(conn);


// 	// redisContext *conn = NULL;
// 	// int return_val = init_redis(&conn);

// 	// set_value("bbbbb", "My armpit hurts?", conn);


// 	// char* value;
// 	// int length; 
// 	// char* res = redisGet("bbbbb", conn, &length);
// 	// printf("%d\n", length);
// 	// printf("%s\n", res);

// 	// close_redis(conn);
// }

