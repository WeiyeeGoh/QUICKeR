#ifndef AES_CTR_H_
#define AES_CTR_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

int ctr_encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext);

int ctr_decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext);

int prg_aes_ctr(uint8_t *buffer, uint8_t *key, unsigned int size);


#if defined(__cplusplus)
}
#endif
#endif /* AES_CTR_H_ */
