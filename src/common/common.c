/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h> // just in case

#include "gopt.h"
#include "common.h"

#ifdef _WIN32
#define DEFAULT_PKCS11_LIBRARY_PATH "C:\\Program Files\\Amazon\\CloudHSM\\lib\\cloudhsm_pkcs11.dll"
#else
#define DEFAULT_PKCS11_LIBRARY_PATH "/opt/cloudhsm/lib/libcloudhsm_pkcs11.so"
#endif

CK_BBOOL true_val = TRUE;
CK_BBOOL false_val = FALSE;


static void show_help(void) {
    printf("\n\t--pin <user:password>\n\t[--library <path/to/pkcs11>]\n");
}


int init_params (const char* filename, struct parameters* args) {
    FILE *fp;

    char *line1 = NULL;
    char* pin = NULL;
    size_t len = 0;
    ssize_t read;

    char* library = "/opt/cloudhsm/lib/libcloudhsm_pkcs11.so";

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("EXIT FAILURE\n");
        exit(EXIT_FAILURE);
    }

    // line 1 is ip address of redis
    // line 2 is port num
    // line 3 is pkcs11 arguments
    // line 4 ...?

    // Get ip_addr
    read = getline(&(args->redis_parameters.ip_addr), &len, fp);
    int ip_addr_length = strlen(args->redis_parameters.ip_addr);
    args->redis_parameters.ip_addr[ip_addr_length-1] = '\0';

    // Get pin
    read = getline(&pin, &len, fp);
    pin[strlen(pin)-1] = '\0';


    args->pkcs_parameters.pin = pin;
    args->pkcs_parameters.library = library;

    // Get port num for redis
    //printf("Before this thing\n");
    char* tmpbuf = NULL;
    read = getline(&tmpbuf, &len, fp);
    tmpbuf[strlen(tmpbuf)-1] = '\0';
    args->redis_parameters.portnum = atoi (tmpbuf);
    free (tmpbuf);

    // Get Data Parameter 
    read = getline(&tmpbuf, &len, fp);
    args->data_parameters.message_size = atoi(tmpbuf);
    tmpbuf[strlen(tmpbuf)-1] = '\0';
    free(tmpbuf);
    read = getline(&tmpbuf, &len, fp);
    args->data_parameters.keys_per_db = atoi(tmpbuf);
    tmpbuf[strlen(tmpbuf)-1] = '\0';
    free(tmpbuf);



    return 0;
}

int get_pkcs_args(int argc, char** argv, struct pkcs_arguments* args) {
    if (!args || !argv || argc == 0) {
        return -1;
    }

    struct option options[5];

    options[0].long_name  = "pin";
    options[0].short_name = 0;
    options[0].flags      = GOPT_ARGUMENT_REQUIRED;

    options[1].long_name  = "library";
    options[1].short_name = 0;
    options[1].flags      = GOPT_ARGUMENT_REQUIRED;

    options[2].long_name  = "wp_key";
    options[2].short_name = 0;
    options[2].flags      = GOPT_ARGUMENT_REQUIRED;

    options[3].long_name  = "object-id";
    options[3].short_name = 0;
    options[3].flags      = GOPT_ARGUMENT_REQUIRED;

    options[4].flags      = GOPT_LAST;


    gopt (argv, options);


    // Check for required argument, pin.
    if (options[0].count != 1) {
        show_help();
        return -1;
    }


    args->pin = options[0].argument;
    args->library = options[1].argument;

    // Default to the standard CloudHSM PKCS#11 library location.
    if (!args->library) {
        args->library = DEFAULT_PKCS11_LIBRARY_PATH;
    }


    args->wrapping_key_handle = CK_INVALID_HANDLE;
    if (options[2].argument) {
        args->wrapping_key_handle = strtoul(options[2].argument, NULL, 0);
    }

    args->object_handle = CK_INVALID_HANDLE;
    if (options[3].argument) {
        args->object_handle = strtoul(options[3].argument, NULL, 0);
    }


    return 0;
}

/**
 * Converts a byte array to a hex string.
 * This function will allocate the appropriate memory for the hex string.
 * If a valid pointer is passed, that pointer will be reallocated. This
 * allows the caller to reuse the same pointer through multiple calls.
 * @param bytes
 * @param bytes_len
 * @param hex
 * @return
 */
int bytes_to_new_hexstring(char *bytes, size_t bytes_len, unsigned char **hex_array) {
    if (!bytes || !hex_array) {
        return -1;
    }

    unsigned char *tmp = realloc(*hex_array, bytes_len * 2 + 1);
    if (!tmp) {
        if (*hex_array) {
            free(*hex_array);
        }
        return -1;
    }
    *hex_array = tmp;
    memset(*hex_array, 0, bytes_len * 2 + 1);

    char values[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    for (size_t i = 0, j = 0; i < bytes_len; i++, j += 2) {
        *((*hex_array) + j) = values[bytes[i] >> 4 & 0x0f];
        *((*hex_array) + j + 1) = values[bytes[i] & 0x0f];
    }

    return 0;
}

/**
 * Prints a byte array as a hex string.
 * @param bytes
 * @param bytes_len
 * @return
 */
int print_bytes_as_hex(char *bytes, size_t bytes_len) {
    if (!bytes || bytes_len < 1) {
        return -1;
    }

    for (size_t i = 0; i < bytes_len; i++) {
        printf("%02X", bytes[i]);
    }
    printf("\n");

    return 0;
}


char** split_by_comma (char *str, int* count) {

        int length = strlen(str);
        char* token;

        char* copy = malloc (length + 1);
        memset (copy, 0, length + 1);
        memcpy (copy, str, length);
        char* rest = copy;
        (*count) = 0;
        while ((token = strtok_r(rest, ",", &rest))) {
                (*count)++;
        }

        char** string_arr = malloc(*count * sizeof(char*));
        memset(copy, 0, length+1);
        strncpy(copy, str, length);
        rest = copy;
        int index = 0;
        while ((token = strtok_r(rest, ",", &rest))) {
                string_arr[index] = token;
                index ++;
        }

        return string_arr;
}

char** split_by_space (char *str, int* count) {

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


char** split_by_char (char *str, int* count, char val) {

        int length = strlen(str);
        char* token;

        char* copy = malloc (length + 1);
        memset (copy, 0, length + 1);
        memcpy (copy, str, length);
        char* rest = copy;
        (*count) = 0;
        while ((token = strtok_r(rest, val + "", &rest))) {
                (*count)++;
        }

        char** string_arr = malloc(*count * sizeof(char*));
        memset(copy, 0, length+1);
        strncpy(copy, str, length);
        rest = copy;
        int index = 0;
        while ((token = strtok_r(rest, val + "", &rest))) {
                string_arr[index] = token;
                index ++;
        }

        return string_arr;
}

struct key_to_db * parse_key_file (char* filename, int* numkeys){
		
	size_t len = 0;
   	ssize_t read;
	char *line = NULL;
    	FILE *fp;
    	fp = fopen(filename, "r");
	
	
	int count = 0;

    	while ((read = getline(&line, &len, fp)) != -1) {
		count ++;
	}
	free(line);
	(*numkeys) = count;

	close (fp);

	struct key_to_db * ret = malloc (sizeof (struct key_to_db) * (count));

	FILE *fp2;
	fp = fopen (filename, "r");
	int j = 0;
    	while ((read = getline(&line, &len, fp)) != -1) {
		int i = 0;
    		char ** key_and_db = split_by_space (line, &i);
		if (i == 2) {
			ret[j].key = key_and_db[0];
			ret[j].db_ip_addr = key_and_db[1];
		}
		j++;
	}
	free(line);
	close (fp2);
	return ret;
}

struct key_to_db get_random_key (struct key_to_db* map, int size) {
	int index = rand() % size;
	return map[index];
}


