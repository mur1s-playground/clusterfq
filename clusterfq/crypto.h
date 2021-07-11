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

	void* mp_0;
	void* mp_1;
};

/* INTERN */

char* crypto_key_private_get_buffer(EVP_PKEY* pkey, int* len);
EVP_PKEY* crypto_key_private_get(char* private_key, int private_key_len);
EVP_PKEY* crypto_key_public_get(struct Key* key);

/* KEYGEN */

extern void crypto_key_init(struct Key* key);

extern struct Key* crypto_key_copy(struct Key* key);
extern void crypto_key_copy(struct Key* key_in, struct Key* key_out);
extern void crypto_key_name_set(struct Key* key, const char* name, int name_len);

extern void crypto_key_private_generate(struct Key* key, int bits);
extern void crypto_key_public_extract(struct Key* key);

extern void crypto_key_reset_internal(struct Key* key);

extern void crypto_key_sym_generate(struct Key* key);
extern void crypto_key_sym_finalise(struct Key* key);

/* SIGN/VERIFY */

char* crypto_sign_message(struct Key* key, char* to_sign, unsigned int to_sign_len, unsigned int* sig_len = nullptr, bool base64 = true);
bool crypto_verify_signature(struct Key* key, char* to_verify, unsigned int to_verify_len, char* base64_signature, unsigned int base64_signature_len, bool base64 = true);

/* ENCRYPTION/DECRYPTION */

extern char* crypto_key_public_encrypt(struct Key* key, char* to_encrypt, int to_encrypt_size, unsigned int *out_size = nullptr);
extern char* crypto_key_private_decrypt(struct Key* key, char* to_decrypt, int to_decrypt_size, int* out_len = nullptr);

extern unsigned char* crypto_key_sym_encrypt(struct Key* key, unsigned char* to_encrypt, int to_encrypt_len, int* len);
extern unsigned char* crypto_key_sym_decrypt(struct Key* key, unsigned char* to_decrypt, int to_decrypt_len, int* len);

/* ENCODE/DECODE */
char* crypto_base64_encode(unsigned char* to_encode, size_t length, bool slash_to_underscore = false);
unsigned char* crypto_base64_decode(const char* to_decode, size_t* out_length, bool underscore_to_slash = false);


/* UTIL */
char* crypto_key_fingerprint(struct Key* pubkey);
extern void crypto_key_dump(struct Key* key);
extern void crypto_key_list_dump(struct Key** key_list, int key_list_len);

extern char* crypto_random(int len);

unsigned char* crypto_hash_sha256(unsigned char* to_hash, int to_hash_len);
unsigned char* crypto_hash_md5(unsigned char* to_hash, int to_hash_len);

extern char* crypto_pad_add(char* message, int message_len, int total_len);
extern char* crypto_pad_remove(char* message, int total_len, int* message_len_out);
