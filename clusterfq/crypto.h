#pragma once

#include <openssl/bio.h>
#include <openssl/pem.h>

struct Key {
	char* name;
	int name_len;

	char* private_key;
	int private_key_len;

	char* public_key;
	int public_key_len;
};

/* INTERN */

char* crypto_key_private_get_buffer(EVP_PKEY* pkey, int* len);
EVP_PKEY* crypto_key_private_get(char* private_key, int private_key_len);
EVP_PKEY* crypto_key_public_get(struct Key* key);

/* KEYGEN */

extern struct Key* crypto_key_copy(struct Key* key);
extern void crypto_key_name_set(struct Key* key, const char* name, int name_len);

extern void crypto_key_private_generate(struct Key* key, int bits);
extern void crypto_key_public_extract(struct Key* key);

extern void crypto_key_sym_generate(struct Key* key);
extern void crypto_key_sym_finalise(struct Key* key);

/* ENCRYPTION/DECRYPTION */

extern char* crypto_key_public_encrypt(struct Key* key, char* to_encrypt, int to_encrypt_size, unsigned int *out_size = nullptr);
extern char* crypto_key_private_decrypt(struct Key* key, char* to_decrypt, int to_decrypt_size, int* out_len = nullptr);

extern unsigned char* crypto_key_sym_encrypt(struct Key* key, unsigned char* to_encrypt, int to_encrypt_len, int* len);
extern unsigned char* crypto_key_sym_decrypt(struct Key* key, unsigned char* to_decrypt, int to_decrypt_len, int* len);

/* UTIL */

extern void crypto_key_dump(struct Key* key);
extern void crypto_key_list_dump(struct Key** key_list, int key_list_len);

extern char* crypto_random(int len);

unsigned char* crypto_hash_sha256(unsigned char* to_hash, int to_hash_len);
unsigned char* crypto_hash_md5(unsigned char* to_hash, int to_hash_len);

extern char* crypto_pad_add(char* message, int message_len, int total_len);
extern char* crypto_pad_remove(char* message, int total_len, int* message_len_out);
