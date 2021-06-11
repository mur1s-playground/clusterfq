#include "crypto.h"

#include "ecdh.h"

#include <string.h>
#include <fcntl.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif

#include <openssl/md5.h>
#include <openssl/bn.h>
#include <openssl/rsa.h>

#ifdef _WIN32
#include <openssl/rand.h>
#else
#include <unistd.h>
#endif

/* INTERNAL */

char* crypto_key_private_get_buffer(EVP_PKEY* pkey, int* len) {
	BIO* bp = BIO_new(BIO_s_mem());
	if (!PEM_write_bio_PrivateKey(bp, pkey, NULL, NULL, 0, 0, NULL)) {
		fprintf(stderr, "ERROR: Unable to write private key bio\n");
		return NULL;
	}
	char* data, * p;
	int size = (int)BIO_get_mem_data(bp, &p);
	*len = size + 1;					/* +1 */
	data = (char*)malloc(size + 1);
	memcpy(data, p, size);
	data[size] = '\0';
	BIO_free(bp);
	return data;
}

EVP_PKEY* crypto_key_private_get(char* private_key, int private_key_len) {
	BIO* privBio = BIO_new(BIO_s_mem());
	BIO_write(privBio, private_key, private_key_len); /* -1 */
	RSA* cipher = PEM_read_bio_RSAPrivateKey(privBio, NULL, NULL, NULL);
	EVP_PKEY* priv = EVP_PKEY_new();
	EVP_PKEY_set1_RSA(priv, cipher);
	return priv;
}

EVP_PKEY* crypto_key_public_get(struct Key* key) {
	BIO* pubBio = BIO_new(BIO_s_mem());
	BIO_write(pubBio, key->public_key, key->public_key_len);
	EVP_PKEY* pub = PEM_read_bio_PUBKEY(pubBio, NULL, NULL, NULL);
	return pub;
}

/* KEYGEN */

struct Key* crypto_key_copy(struct Key* key) {
	struct Key* ret;
	ret = (struct Key*)malloc(sizeof(struct Key));
	ret->name = (char*)malloc(key->name_len + 1);
	memcpy((void*)ret->name, (void*)key->name, key->name_len);
	/*	strncpy(ret->name, key->name, key->name_len);*/
	ret->name[key->name_len] = '\0';
	ret->name_len = key->name_len;
	ret->private_key = (char*)malloc(key->private_key_len);
	memcpy((void*)ret->private_key, (void*)key->private_key, key->private_key_len);
	/*	strncpy(ret->private_key, key->private_key, key->private_key_len);*/
	ret->private_key_len = key->private_key_len;
	ret->public_key = (char*)malloc(key->public_key_len);
	memcpy((void*)ret->public_key, (void*)key->public_key, key->public_key_len);
	/*	strncpy(ret->public_key, key->public_key, key->public_key_len);*/
	ret->public_key_len = key->public_key_len;
	return ret;
}

void crypto_key_name_set(struct Key* key, const char* name, int name_len) {
	key->name = (char*)malloc(name_len + 1);
	memcpy(key->name, name, name_len);
	key->name[name_len] = '\0';
	key->name_len = name_len;
}

void crypto_key_private_generate(struct Key* key, int bits) {
	EVP_PKEY* pkey;
	pkey = EVP_PKEY_new();
	RSA* rsa = RSA_new();
	BIGNUM *e;
	e = BN_new();
	BN_set_word(e, RSA_F4);
	RSA_generate_key_ex(rsa, bits, e, NULL);
	if (rsa == NULL) {
		fprintf(stderr, "ERROR: Unable to create rsa private key\n");
		key->private_key_len = 0;
		return;
	}
	EVP_PKEY_assign_RSA(pkey, rsa);
	key->private_key = crypto_key_private_get_buffer(pkey, &(key->private_key_len));
}

void crypto_key_public_extract(struct Key* key) {
	EVP_PKEY* pkey = crypto_key_private_get(key->private_key, key->private_key_len);
	BIO* bp = BIO_new(BIO_s_mem());
	int ret;
	ret = PEM_write_bio_PUBKEY(bp, pkey);
	if (ret == 0) {
		fprintf(stderr, "ERROR: Unable to extract public key\n");
		key->public_key_len = 0;
		BIO_free(bp);
		return;
	}
	char* p;
	int size = (int)BIO_get_mem_data(bp, &p);
	key->public_key_len = size + 1;
	key->public_key = (char*)malloc(size + 1);
	memcpy(key->public_key, p, size);
	key->public_key[size] = '\0';
	BIO_free(bp);
}

void crypto_key_sym_generate(struct Key* key) {
	key->private_key = crypto_random(ECC_PRV_KEY_SIZE);
	key->private_key_len = ECC_PRV_KEY_SIZE;
	key->public_key = (char*)malloc(ECC_PUB_KEY_SIZE);
	ecdh_generate_keys((uint8_t *)key->public_key, (uint8_t *)key->private_key);
	key->public_key_len = ECC_PUB_KEY_SIZE;
}

void crypto_key_sym_finalise(struct Key* key) {
	char* sym_key, * old_pt;
	sym_key = (char*)malloc(ECC_PUB_KEY_SIZE);
	ecdh_shared_secret((uint8_t*)key->private_key, (uint8_t*)key->public_key, (uint8_t *)sym_key);
	key->public_key_len = 0;
	free(key->public_key);
	old_pt = key->private_key;
	key->private_key = sym_key;
	free(old_pt);
	key->private_key_len = ECC_PUB_KEY_SIZE;
}


/* ENCRYPTION/DECRYPTION */

char* crypto_key_public_encrypt(struct Key* key, char* to_encrypt, int to_encrypt_size, unsigned int* out_size) {
	RSA* rsa = NULL;
	BIO* keybio = BIO_new(BIO_s_mem());
	EVP_PKEY* pkey = crypto_key_public_get(key);
	PEM_write_bio_PUBKEY(keybio, pkey);
	if (!keybio) {
		fprintf(stderr, "ERROR: failed to create key BIO\n");
		return NULL;
	}
	rsa = PEM_read_bio_RSA_PUBKEY(keybio, NULL, NULL, NULL);
	if (!rsa) {
		fprintf(stderr, "ERROR: failed to create RSA\n");
		return NULL;
	}
	BIO_free(keybio);

	EVP_PKEY* verification_key = EVP_PKEY_new();
	int rc = EVP_PKEY_assign_RSA(verification_key, RSAPublicKey_dup(rsa));
	if (rc != 1) {
		fprintf(stderr, "ERROR: rc != 1\n");
		return NULL;
	}
	if (verification_key) EVP_PKEY_free(verification_key);
	int size = RSA_size(rsa);
	if (out_size != nullptr) *out_size = (unsigned int)size;
	//fprintf(stdout, "rsa_size %i\n", size);
	if (to_encrypt_size >= size - 41) { /* Padding: rsa size of 512 and message size >= 471 fails */
		fprintf(stderr, "ERROR: to_encrypt too long: %i >= %i-41\n", to_encrypt_size, size);
		return NULL;
	}
	unsigned char* encrypted;
	encrypted = (unsigned char*)malloc((size + 1) * sizeof(unsigned char));
	memset(encrypted, 0, (size + 1) * sizeof(unsigned char));
	int ret = RSA_public_encrypt(to_encrypt_size, (unsigned char*)to_encrypt, encrypted, rsa, RSA_PKCS1_OAEP_PADDING);
	if (ret == -1) {
		fprintf(stderr, "ERROR: Unable to encrypt\n");
		return NULL;
	}
	encrypted[size] = '\0';
	return (char *)encrypted;
}

char* crypto_key_private_decrypt(struct Key* key, char* to_decrypt, int to_decrypt_size, int* out_len) {
	RSA* rsa = NULL;
	BIO* keybio = BIO_new(BIO_s_mem());
	EVP_PKEY* pkey = crypto_key_private_get(key->private_key, key->private_key_len);
	PEM_write_bio_PrivateKey(keybio, pkey, NULL, NULL, 0, 0, NULL);
	if (!keybio) {
		fprintf(stderr, "ERROR: Unable to create key BIO\n");
		return nullptr;
	}
	rsa = PEM_read_bio_RSAPrivateKey(keybio, NULL, NULL, NULL);
	if (!rsa) {
		fprintf(stderr, "ERROR: Unable tocreate RSA\n");
		return nullptr;
	}
	BIO_free(keybio);

	EVP_PKEY* verification_key = EVP_PKEY_new();
	int rc = EVP_PKEY_assign_RSA(verification_key, RSAPrivateKey_dup(rsa));
	if (rc != 1) {
		fprintf(stderr, "ERROR: rc != 1\n");
		return nullptr;
	}
	if (verification_key) EVP_PKEY_free(verification_key);
	unsigned char* decrypted = (unsigned char*)malloc((to_decrypt_size + 1) * sizeof(unsigned char));
	memset(decrypted, 0, (to_decrypt_size + 1) * sizeof(unsigned char));
	int ret = RSA_private_decrypt(to_decrypt_size, (unsigned char*)to_decrypt, decrypted, rsa, RSA_PKCS1_OAEP_PADDING);
	if (ret == -1) {
		fprintf(stderr, "ERROR: decrypting\n");
		free(decrypted);
		return nullptr;
	}
	decrypted = (unsigned char*)realloc(decrypted, (ret + 1) * sizeof(unsigned char));
	decrypted[ret] = '\0';
	if (out_len != nullptr) *out_len = ret;
	return (char*)decrypted;
}

unsigned char* crypto_key_sym_encrypt(struct Key* key, unsigned char* to_encrypt, int to_encrypt_len, int* len) {
	int split = (2.0 / 3.0) * key->private_key_len;
	unsigned char* hashed_key = crypto_hash_sha256((unsigned char *)key->private_key, split);
	unsigned char* hashed_iv = crypto_hash_md5((unsigned char*)&(key->private_key[split - 1]), key->private_key_len - split);
	unsigned char* encrypted = (unsigned char*)malloc(to_encrypt_len + 128 - (to_encrypt_len % 128));
	int encrypted_len;
	EVP_CIPHER_CTX* ctx;
	if (!(ctx = EVP_CIPHER_CTX_new())) {
		printf("error creating cipher context\n");
		return NULL;
	}
	if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, hashed_key, hashed_iv)) {
		printf("error init enc\n");
		return NULL;
	}
	if (1 != EVP_EncryptUpdate(ctx, encrypted, &encrypted_len, to_encrypt, to_encrypt_len)) {
		printf("error encrypting data\n");
		return NULL;
	}
	*len = encrypted_len;
	if (1 != EVP_EncryptFinal_ex(ctx, encrypted + encrypted_len, &encrypted_len)) {
		printf("error finalizing\n");
		return NULL;
	}
	*len = *len + encrypted_len;
	EVP_CIPHER_CTX_free(ctx);
	free(hashed_key);
	free(hashed_iv);
	return encrypted;
}

unsigned char* crypto_key_sym_decrypt(struct Key* key, unsigned char* to_decrypt, int to_decrypt_len, int* len) {
	int split = (2.0 / 3.0) * key->private_key_len;
	unsigned char* hashed_key = crypto_hash_sha256((unsigned char*)key->private_key, split);
	unsigned char* hashed_iv = crypto_hash_md5((unsigned char*)&(key->private_key[split - 1]), key->private_key_len - split);
	EVP_CIPHER_CTX* ctx;
	unsigned char* decrypted = (unsigned char*)malloc(to_decrypt_len);
	int decrypted_len;
	if (!(ctx = EVP_CIPHER_CTX_new())) {
		printf("error creating cipher context\n");
		return nullptr;
	}
	if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, hashed_key, hashed_iv)) {
		printf("error init dec\n");
		return nullptr;
	}
	if (1 != EVP_DecryptUpdate(ctx, decrypted, &decrypted_len, to_decrypt, to_decrypt_len)) {
		printf("error decrypting\n");
		return nullptr;
	}
	*len = decrypted_len;
	if (1 != EVP_DecryptFinal_ex(ctx, decrypted + decrypted_len, &decrypted_len)) {
		printf("error finalizing dec\n");
		return nullptr;
	}
	*len = *len + decrypted_len;
	decrypted[(*len)] = '\0';
	EVP_CIPHER_CTX_free(ctx);
	return decrypted;
}



/* UTILS */

void crypto_key_dump(struct Key* key) {
	int i;
	printf("/--- KEY DUMP -----------------------------------\n");
	printf("/ name            : %s\n", key->name);
	printf("/ name_len        : %d\n", key->name_len);
	printf("/ private_key_len : %d\n", key->private_key_len);
	if (key->private_key_len > 0) {
		printf("/ private_key     : ");
		for (i = 0; i < key->private_key_len / 2; i++) {
			fprintf(stdout, "%02x", (unsigned char)key->private_key[i * 2]);
		}
		fprintf(stdout, "\n");
	}
	printf("/ public_key_len  : %d\n", key->public_key_len);
	if (key->public_key_len > 0) {
		printf("/ public_key      : ");
		for (i = 0; i < key->public_key_len / 2; i++) {
			fprintf(stdout, "%02x", (unsigned char)key->public_key[i * 2]);
		}
		fprintf(stdout, "\n");
	}
	printf("/------------------------------------------------\n");

}

void crypto_key_list_dump(struct Key** key_list, int key_list_len) {
	int i;
	for (i = 0; i < key_list_len; i++) {
		if (key_list[i] != NULL) {
			crypto_key_dump(key_list[i]);
		}
	}
}

char* crypto_random(int len) {
	char* result;
	result = (char*)malloc(len);
#ifdef _WIN32
	int ret_b = RAND_bytes((unsigned char*)result, len);
	if (ret_b == 0) {
		fprintf(stderr, "ERROR: Unable to generate random numbers\n");
		return NULL;
	}
#else
	int random_data = open("/dev/urandom", O_RDONLY);
	if (random_data < 0) {
		fprintf(stderr, "ERROR: Unable to open random number source\n");
		return NULL;
	}
	size_t ret = read(random_data, result, len);
	if (ret < 0) {
		fprintf(stderr, "ERROR: Unable to read random numbers\n");
		return NULL;
	}
#endif
	return result;
}

unsigned char* crypto_hash_sha256(unsigned char* to_hash, int to_hash_len) {
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	unsigned char* hash = (unsigned char*)malloc((SHA256_DIGEST_LENGTH + 1) * sizeof(unsigned char));
	SHA256_Update(&sha256, to_hash, to_hash_len);
	SHA256_Final(hash, &sha256);
	hash[SHA256_DIGEST_LENGTH] = '\0';
	return hash;
}

unsigned char* crypto_hash_md5(unsigned char* to_hash, int to_hash_len) {
	MD5_CTX c;
	unsigned char* hash = (unsigned char*)malloc(17 * sizeof(unsigned char));
	MD5_Init(&c);
	MD5_Update(&c, to_hash, to_hash_len);
	MD5_Final(hash, &c);
	hash[17] = '\0';
	return hash;
}

char* crypto_pad_add(char* message, int message_len, int total_len) {
	char* result;
	char* hash;
	int j;
	if (message_len >= total_len - 2 - SHA256_DIGEST_LENGTH) return NULL;
	result = crypto_random(total_len);
	int start_index = total_len - 1;
	while (start_index + message_len + 2 + SHA256_DIGEST_LENGTH > total_len) {
		start_index = (int)(crypto_random(1))[0];
	}
	for (int i = 0; i < start_index; i++) {
		while (result[i] == ':') {
			result[i] = (crypto_random(1))[0];
		}
	}
	result[start_index] = ':';
	for (int i = 0; i < message_len; i++) {
		result[start_index + 1 + i] = message[i];
	}
	result[start_index + 1 + message_len] = ':';
	for (int i = start_index + 1 + message_len + 1; i < total_len - SHA256_DIGEST_LENGTH; i++) {
		while (result[i] == ':') {
			result[i] = (crypto_random(1))[0];
		}
	}
	hash = (char *)crypto_hash_sha256((unsigned char*)result, total_len - SHA256_DIGEST_LENGTH);
	j = 0;
	for (int i = total_len - 1; i > total_len - SHA256_DIGEST_LENGTH; i--) {
		result[i] = hash[SHA256_DIGEST_LENGTH - 1 - j];
		j++;
	}
	return result;
}

char* crypto_pad_remove(char* message, int total_len, int* message_len_out) {
	char* result;
	char* hash;
	int j;
	hash = (char *)crypto_hash_sha256((unsigned char*)message, total_len - SHA256_DIGEST_LENGTH);
	j = 0;
	for (int i = total_len - 1; i > total_len - SHA256_DIGEST_LENGTH; i--) {
		if (message[i] != hash[SHA256_DIGEST_LENGTH - 1 - j]) {
			fprintf(stderr, "ERROR: crypto_pad sha not matching\n");
			return NULL;
		}
		j++;
	}
	int last, started, len;
	result = (char*)malloc(total_len);
	started = 0;
	for (int i = total_len - 1 - SHA256_DIGEST_LENGTH; i >= 0; i--) {
		if (message[i] == ':') {
			last = i;
			break;
		}
	}
	len = 0;
	for (int i = 0; i < last; i++) {
		if (started == 0) {
			if (message[i] == ':') {
				started = 1;
			}
		}
		else {
			result[len] = message[i];
			len++;
		}
	}
	result = (char*)realloc(result, len + 1);
	result[len] = '\0';
	(*message_len_out) = len;
	return result;
}
