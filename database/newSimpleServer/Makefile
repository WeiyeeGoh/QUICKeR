all:
	gcc -o simple-server -g -lpthread -lssl -lcrypto simple_server.c AE_Nested_AES.c aes_ctr.c aes_gcm.c libhiredis.a
	gcc -o simple-client simple_client.c
clean:
	rm simple-server simple-client
