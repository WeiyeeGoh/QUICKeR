#include <hiredis/hiredis.h>
#include <stdio.h>
#include <string.h>
#include "redis_storage.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>

double store_time = 0.0;
double retrieve_time = 0.0;


int update_ip_addr (char* newip_addr) {
	ip_addr = newip_addr;
}


int update_portnum (int new_portnum) {
	portnum = new_portnum;
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

	buf = malloc(sizeof(char*) * ((bptr->length) + 100));

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


	if (!(buf = malloc(len + 1)))
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

int recvall (int sockfd, void* recvbuf, int buffsize) {
	//bzero (recvbuf, buffsize);
	int total_bytes = 0;
	int nbytes = 0;

	void* startbuf = recvbuf;


	int index = 0;
	while (total_bytes < buffsize && (nbytes = recv(sockfd, startbuf, buffsize, 0)) > 0){
		// From here, valid bytes are from recvbuf to recvbuf + nbytes.
		// You could simply fwrite(fp, recvbuf, nbytes) or similar. 
		//nbytes = recv(sockfd, startbuf, buffsize - total_bytes, 0);

		startbuf += nbytes;
		total_bytes += nbytes;
		index += 1;
		//printf (" -- Iteration %d -- \n", index);
		//printf ("Total_bytes: %d\n", total_bytes);
		//printf ("nbytes : %d\n", nbytes);
		//printf ("buffsize: %d\n", buffsize);

		if (((char*)recvbuf)[total_bytes-1] == '\0') {
			break;
		}
		//sleep (0.2);
	}
	//printf ("Total_bytes: %d\n", total_bytes);
	//printf ("nbytes : %d\n", nbytes);
	//printf ("buffsize: %d\n", buffsize);
	if (nbytes == -1) {
		perror ("recv");
	}
	//printf ("Received: %s\n", (char*)recvbuf);
	((char*)recvbuf)[total_bytes+1] = 0;
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


int get_sockfd () {
	return sockfd;
}

int init_redis (redisContext **conn) {
	struct sockaddr_in servaddr, cli;

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("socket creation failed...\n");
		exit(0);
	} 
	// else {
	//         printf("Socket successfully created..\n");
	// }


	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(ip_addr);
	servaddr.sin_port = htons(portnum);


	// connect the client socket to server socket
	int connect_rv = connect(sockfd, (SA*)&servaddr, sizeof(servaddr));
	int connect_attempt = 10;
	while (connect_rv != 0) {
		connect_attempt -= 1;
		connect_rv = connect(sockfd, (SA*)&servaddr, sizeof(servaddr));
		sleep(1);
		if (connect_attempt == 0) {
			printf("connection with the server failed on port %d...\n", portnum);
			printf("Exit Code: %d\n", connect_rv);
			printf("\n");
			exit(0);
		}
	}
	// else {
	//         printf("connected to the server on port %d..\n", portnum);
	// }

	return 0;
}

int close_redis (redisContext *conn) {


	close(sockfd);
	return 0;
}

int savedata(char *value, redisContext *conn) {
	init_redis (conn);

	int size = 9 + strlen (value) + 1;

	buff[0] = 0;
	strncat (buff, "SAVEDATA ", 9);
	strncat (buff, value, strlen(value));

	buff[size] = 0;


	sendall(sockfd, buff, size);

	recvall(sockfd, buff, MAX);

	close (sockfd);
	return 0;
}

int populate(char *value, redisContext *conn) {
	init_redis (conn);

	int size = 9 + strlen (value) + 1;

	buff[0] = 0;
	strncat (buff, "POPULATE ", 9);
	strncat (buff, value, strlen(value));

	buff[size] = 0;


	sendall(sockfd, buff, size);

	recvall(sockfd, buff, MAX);

	close (sockfd);
	return 0;
}


int set(char *key, char *value, int value_length, redisContext *conn) {

	init_redis (conn);
	char* b64encoded = base64_enc(value, value_length);


	int size = 4 + strlen (key) + 1 + strlen (b64encoded)+ 1;

	buff[0] = 0;
	strncat (buff, "SET ", 4);
	strncat (buff, key, strlen(key));
	strncat (buff, " ", 1);
	strncat (buff, b64encoded, strlen(b64encoded));

	buff[size] = 0;


	sendall(sockfd, buff, size);

	recvall(sockfd, buff, MAX);

	free (b64encoded);
	close (sockfd);
	return 0;
}


char* get(char *key, redisContext *conn, int *length) {

	init_redis (conn);
	int size = 4 + strlen (key) + 1;
	buff[0] = 0;
	strncat (buff, "GET ", 4);
	strncat (buff, key, strlen(key));    
	buff[size+1] = 0;

	sendall(sockfd, buff, size);
	//buff[0] = 0;
	recvall(sockfd, buff, MAX);


	int result_length;
	char* b64decoded = base64_dec(buff, strlen(buff), &result_length);
	*length = result_length;

	if (*length <= 0) {
		printf ("Length <= 0\n");
	}

	close (sockfd);
	return b64decoded;   
}


char** getall (int numkeys, char** keys, int**sizes, redisContext *conn) {
	init_redis(conn);
	
	int size = 7;
	buff[0] = 0;
	strncat (buff, "GETALL ", size);

	for (int i = 0; i < numkeys; i++) {
		strncat (buff, keys[i], strlen(keys[i]));    
		if (i < numkeys -1) {
			strncat (buff, " ", 1);
		}
		size += strlen (keys[i]) + 1;
		//buff[size+1] = 0;
	}
	//buff[size+1] = 0;

	//printf("Sent Data: %s\n", buff);
	sendall(sockfd, buff, size);
	recvall(sockfd, buff, MAX);
	
	char ** ret = malloc (numkeys * sizeof (char *));
	
	int count = 0;
	char ** values = split_by_space (buff, &count);
	*sizes = malloc(numkeys);
	//printf("addr: %d\n", *sizes);

	printf("strlen of data: %d\n", strlen(values[0]));

	for (int i = 0; i < numkeys; i++) {
		int result_length;
		ret[i] = base64_dec(values[i], strlen(values[i]), &result_length);

		(*sizes)[i] = result_length;
		//printf("reslength: %d\n", (*sizes)[i]);

		if (result_length <= 0) {
			printf ("Length <= 0\n");
		}
	}
	free (values[0]);
	free (values);

	for (int i = 0; i < numkeys; i++) {
		printf("size: %d\n", (*sizes)[i]);
	}


	close (sockfd);
	return ret;   
}

int setall (int numkeys, char** keys, char** values, int** value_sizes, redisContext *conn) {

	init_redis(conn);

	int size = 7;
	buff[0] = 0;
	strncat (buff, "SETALL ", size);

	for (int i = 0; i < numkeys; i++) {

		char* b64encoded = base64_enc(values[i], value_sizes[i]);

		if (i == 3) {
			printf("SET KEY: %s\n", keys[i]);
	    	printf("SET VALUE: %s\n", b64encoded);
	    }

		strncat (buff, keys[i], strlen(keys[i]));    
		strncat (buff, " ", 1);
		strncat (buff, b64encoded, strlen(b64encoded));
		if (i < numkeys -1) {
			strncat (buff, " ", 1);
		}
		size += strlen (keys[i]) + strlen(b64encoded) + 2;

		free(b64encoded);
	}
	sendall(sockfd, buff, size);
	recvall(sockfd, buff, MAX);

	close (sockfd);
	return 1;
}

int send_root_key (int root_key, redisContext *conn) {

	init_redis(conn);

	int size = 8;
	buff[0] = 0;
	strncat (buff, "ROOTKEY ", size);


    char root_key_string[15];
    sprintf(root_key_string, "%d", root_key);
    printf("root_key_string: %s\n", root_key_string);
    char* b64_root_key = base64_enc((char*) root_key_string, 15);
    printf("b64_root_key: %s\n", b64_root_key);


	strncat (buff, b64_root_key, strlen(b64_root_key));
	size += strlen(b64_root_key) + 2;

	sendall(sockfd, buff, size);
	recvall(sockfd, buff, MAX);

	printf("GOD BACK STUFF\n");


	free(b64_root_key);
	close (sockfd);
	return 1;
}



int timed_set(char *key, char *value, int value_length, redisContext *conn) {
	return set(key, value, value_length, conn);
}

double get_timed_set() {
	return 0.01;
}


int reset_database(redisContext *conn) {
	return 0;
}
