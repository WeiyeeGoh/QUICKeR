#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>


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

//#define MAX 10000000
#define MAX 20000
#define PORT 6000
#define SA struct sockaddr


char key_arr[2][20] = {
    "key_aaaaa",
    "key_bbbbb",
};

char val_arr[2][20] = {
    "george_eats_food",
    "george_hates_food",
};


int recvall (int sockfd, void* recvbuf, int buffsize) {
        bzero (recvbuf, buffsize);
        int total_bytes = 0;
        int nbytes = 0;

        void* startbuf = recvbuf;


        int index = 0;
        while (total_bytes < buffsize && (nbytes = recv(sockfd, startbuf, 8192, 0)) > 0){
                // From here, valid bytes are from recvbuf to recvbuf + nbytes.
                // You could simply fwrite(fp, recvbuf, nbytes) or similar. 
                startbuf += nbytes;
                total_bytes += nbytes;
                index += 1;

                if (((char*)recvbuf)[total_bytes-1] == '\0') {
                        break;
                }
        }
        return total_bytes;
}

int sendall (int sockfd, void* sendbuf, int sendsize) {
	int total_bytes = 0;
	int nbytes = 0;

	void* startbuf = sendbuf;

	while (sendsize > 0 && (nbytes = send(sockfd, startbuf, sendsize, 0)) > 0 ) {
		startbuf += nbytes;
		sendsize -= nbytes;
		total_bytes += nbytes;
	}
	printf ("Sent: %s\n", (char*) sendbuf);
	return total_bytes;
}


void test_get(int sockfd, int option)
{
	char buff[MAX];
	int n;

	bzero(buff, sizeof(buff));
	n = 0;

	char* input = "GET ";
	strncat(buff, input, strlen(input));
	strncat(buff, key_arr[option], strlen(key_arr[option]));

	sendall(sockfd, buff, strlen(buff));

	bzero(buff, sizeof(buff));
	recvall(sockfd, buff, sizeof(buff));
	printf("From Server : %s\n", buff);
}


void test_set(int sockfd, int option)
{
	char buff[MAX];
	int n;

	bzero(buff, sizeof(buff));
	n = 0;

	char* input = "SET ";
	strncat(buff, input, strlen(input));
	strncat(buff, key_arr[option], strlen(key_arr[option]));
	strncat(buff, " ", 1);
	strncat(buff, val_arr[option], strlen(val_arr[option]));

	printf ("Before send\n");
	sendall(sockfd, buff, strlen(buff));

	bzero(buff, sizeof(buff));

	printf ("Before recv\n");
	recvall(sockfd, buff, sizeof(buff));
	printf("From Server : %s\n", buff);
}
		

void test_kill(int sockfd, int option) {
	char buff[MAX];
	int n;

	bzero(buff, sizeof(buff));
	n = 0;

	char* input = "KILLME ";
	strncat(buff, input, strlen(input));

	printf ("Before send\n");
	sendall(sockfd, buff, strlen(buff));

	close(sockfd);

	//bzero(buff, sizeof(buff));

	// printf ("Before recv\n");
	// recvall(sockfd, buff, sizeof(buff));
	// printf("From Server : %s\n", buff);
}

// void test_close(int sockfd)
// {
//     char buff[MAX];
//     int n;

//     bzero(buff, sizeof(buff));
//     n = 0;

//     char* input = "CLOSE";
//     strncat(buff, input, strlen(input));

//     sendall(sockfd, buff, strlen(buff));
// }

int connect_to_server(port) {
	int sockfd;
	struct sockaddr_in servaddr, cli;

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("socket creation failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully created..\n");
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(port);


	// connect the client socket to server socket
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
		printf("connection with the server failed on port %d...\n", port);
		exit(0);
	}
	else
		printf("connected to the server on port %d..\n", port);

	return sockfd;
}

double get_time_in_seconds() {
    struct timeval  tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec) + (tv.tv_usec) / 1000000.0 ;
}


int main()
{
	int sockfd = connect_to_server(PORT);

	// // function for chat
	printf ("Before set.\n");
	test_set(sockfd, 0);
	close(sockfd);

	sleep(2);

	
	printf ("Before Get.\n");
	test_get(sockfd, 0);
	close(sockfd);
	
}
