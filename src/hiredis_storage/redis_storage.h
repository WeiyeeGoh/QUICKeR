#ifndef __REDIS_STORAGE_H__
#define __REDIS_STORAGE_H__

#include <hiredis/hiredis.h>

#define SA struct sockaddr
#define MAX 16000000

int sockfd;
char* ip_addr;
int portnum;

char buff[MAX];

int recvall (int sockfd, void* recvbuf, int buffsize);
int sendall (int sockfd, void* sendbuf, int sendsize);
int get_sockfd ();

int init_redis (redisContext **conn);
int close_redis (redisContext *conn);
int set(char *key, char *value, int value_length, redisContext *conn);
char* get(char *key, redisContext *conn, int *length);

char** getall(int numkeys, char **keys, int** sizes, redisContext *conn);
int setall (int numkeys, char** keys, char** values, int** value_sizes, redisContext *conn);

int timed_set(char *key, char *value, int value_length, redisContext *conn);
double get_timed_set();

int reset_database(redisContext *conn);

#endif
