#ifndef _AES_WRAPPING_COMMON_H_
#define _AES_WRAPPING_COMMON_H_

CK_RV aes_gcm_sample(CK_SESSION_HANDLE session);

CK_RV hsm_aes_encrypt(	CK_SESSION_HANDLE session, 
						CK_OBJECT_HANDLE wrapping_key, 
						CK_BYTE_PTR plaintext, 
						CK_ULONG plaintext_length, 
						CK_BYTE_PTR *ciphertext, 
						CK_ULONG *ciphertext_length);

CK_RV hsm_aes_decrypt(	CK_SESSION_HANDLE session, 
						CK_OBJECT_HANDLE wrapping_key, 
						CK_BYTE_PTR ciphertext, 
						CK_ULONG ciphertext_length, 
                        CK_BYTE_PTR *decrypted_ciphertext, 
                        CK_ULONG *decrypted_ciphertext_length);




#endif  
