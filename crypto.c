#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>

#define BUFFER_SIZE 4096

//int main(void) {
//
////	// ------------------------ Test Encryption ------------------------
////	unsigned char iv[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
////			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
////	unsigned char key[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
////			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
////
////	unsigned char inbuf[] = { 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20,
////			0x61, 0x20, 0x74, 0x6F, 0x70, 0x20, 0x73, 0x65,
////			0x63, 0x72, 0x65, 0x74, 0x2E };
////	int inlen = sizeof(inbuf);
////	unsigned char outbuf[1024 + EVP_MAX_BLOCK_LENGTH];
////	int outlen = 0;
////
////
////	unsigned char outbuf2[1024 + EVP_MAX_BLOCK_LENGTH];
////	int outlen2 = 0;
////
////
////
////	// encrypt
////	do_aes_128_cbc_crypt(inbuf, inlen, outbuf, &outlen, key, iv, 1);
////	// decrypt
////	do_aes_128_cbc_crypt(outbuf, outlen, outbuf2, &outlen2, key, iv, 0);
////
////
////	print_buffer(outbuf2, outlen2);
////	printf("the sum is %i \n", outlen2);
//
//
////	// ------------------------ Test Hash ------------------------
////	char inbuf[] = {0x6D, 0x79, 0x20, 0x6E, 0x61, 0x6D, 0x65, 0x20, 0x69, 0x73, 0x20, 0x42, 0x69, 0x6C, 0x61, 0x6C};
////	int inlen = sizeof(inbuf);
////	char outbuf[16];
////	int* outlen;
////	calculate_md5_hash(inbuf, inlen, outbuf, &outlen);
////	print_buffer(outbuf, outlen);
//
//
//
////	// ------------------------ Test HMAC ------------------------
////	unsigned char key[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
////				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
////	unsigned char inbuf[] = {0x6D, 0x79, 0x20, 0x6E, 0x61, 0x6D, 0x65, 0x20, 0x69, 0x73, 0x20, 0x42, 0x69, 0x6C, 0x61, 0x6C};
////	int inlen = sizeof(inbuf);
////	unsigned char outbuf[1024];
////	int outlen = 0;
////	calculate_sha256_hmac(inbuf, inlen, outbuf, &outlen, key);
////	print_buffer(outbuf, outlen);
//
//
//	return 0;
//}

int do_aes_128_cbc_crypt(unsigned char inbuf[], int inlen, unsigned char outbuf[], int* outlen,
		unsigned char key[], unsigned char iv[], int do_encrypt) {
	int outlen1, outlen2;
	EVP_CIPHER_CTX ctx;

	/* Don't set key or IV right away; we want to check lengths */
	EVP_CIPHER_CTX_init(&ctx);
	EVP_CipherInit_ex(&ctx, EVP_aes_128_cbc(), NULL, NULL, NULL, do_encrypt);
	OPENSSL_assert(EVP_CIPHER_CTX_key_length(&ctx) == 16);
	OPENSSL_assert(EVP_CIPHER_CTX_iv_length(&ctx) == 16);

	/* Now we can set key and IV */
	EVP_CipherInit_ex(&ctx, NULL, NULL, key, iv, do_encrypt);

	if (!EVP_CipherUpdate(&ctx, outbuf, &outlen1, inbuf, inlen)) {
		/* Error */
		EVP_CIPHER_CTX_cleanup(&ctx);
		return 0;
	}
	if (!EVP_CipherFinal_ex(&ctx, outbuf + outlen1, &outlen2)) {
		/* Error */
		EVP_CIPHER_CTX_cleanup(&ctx);
		return 0;
	}
	EVP_CIPHER_CTX_cleanup(&ctx);
	*outlen = outlen1 + outlen2;
	return 1;
}

//calculate_sha256_hmac(unsigned char inbuf[], int inlen, unsigned char outbuf[], int* outlen, unsigned char key[]) {
//	HMAC(EVP_sha256(), key, 16, inbuf, inlen, outbuf, outlen);
//}

void calculate_md5_hash(unsigned char inbuf[], int inlen, unsigned char outbuf[], int* outlen) {
	EVP_MD_CTX *mdctx;
	const EVP_MD *md;
	OpenSSL_add_all_digests();
	md = EVP_get_digestbyname("md5");
	mdctx = EVP_MD_CTX_create();
	EVP_DigestInit_ex(mdctx, md, NULL);
	EVP_DigestUpdate(mdctx, inbuf, inlen);
	EVP_DigestFinal_ex(mdctx, outbuf, outlen);
	EVP_MD_CTX_destroy(mdctx);
	/* Call this once before exit. */
	EVP_cleanup();
}

void calculate_sha256_hash(unsigned char inbuf[], int inlen, unsigned char outbuf[], int* outlen) {
	EVP_MD_CTX *mdctx;
	const EVP_MD *md;
	OpenSSL_add_all_digests();
	md = EVP_get_digestbyname("sha256");
	mdctx = EVP_MD_CTX_create();
	EVP_DigestInit_ex(mdctx, md, NULL);
	EVP_DigestUpdate(mdctx, inbuf, inlen);
	EVP_DigestFinal_ex(mdctx, outbuf, outlen);
	EVP_MD_CTX_destroy(mdctx);
	/* Call this once before exit. */
	EVP_cleanup();
}

void calculate_sha256_hash_with_salt(unsigned char salt[],
		int saltlen, unsigned char inbuf[], int inlen, unsigned char outbuf[], int* outlen) {
	unsigned char new_inbuf[BUFFER_SIZE];
	int index = 0;
	memcpy(&new_inbuf[index], &salt[0], saltlen);
	index += saltlen;
	memcpy(&new_inbuf[index], &inbuf[0], inlen);
	index += inlen;
	int new_inlen = index;
	calculate_sha256_hash(new_inbuf, new_inlen, outbuf, outlen);
}


void print_buffer(unsigned char buf[], int buflen) {
	int i;
	printf("------------------");
	for (i = 0; i < buflen; i++) {
		if (i % 16 == 0) {
			printf("\n");
		} else if (i > 0) {
			printf(":");
		}
		printf("%02X", buf[i]);
	}
	printf("\n------------------\n \n");
}

void print_buffer_with_title(unsigned char buf[], int buflen, char* title) {
	int i;
	printf("---------- %s ----------", title);
	for (i = 0; i < buflen; i++) {
		if (i % 16 == 0) {
			printf("\n");
		} else if (i > 0) {
			printf(":");
		}
		printf("%02X", buf[i]);
	}
	printf("\n---------- %s ----------\n \n", title);
}

int compare_buffers(unsigned char buf1[], unsigned char buf2[], int buflen) {
	int result = 1;  // 0 means false, 1 means true
	int i;
	for (i = 0; i < buflen; i++) {
		if (buf1[i] != buf2[i]) {
			result = 0;
			return result;
		}
	}
	return result;
}

void generate_random_number(unsigned char generated_number[], int len){
	FILE* random = fopen("/dev/urandom", "r");
	fread(generated_number, sizeof(unsigned char)*len, 1, random);
	fclose(random);
}
int find_char_in_string(char* str, char c){
	int result = -1;
	const char *ptr = strchr(str, c);
	if(ptr) {
	   result = ptr - str;
	}
	return result;
}

void convert_hex_string_to_bytes_array (char* str, unsigned char buf[]){
	const char *pos = str;
	size_t count = 0;
	int bufln = strlen(str) / 2 ;
	for (count = 0; count < bufln; count++) {
		char element_buf[5] = {'0', 'x', pos[0], pos[1], 0};
		buf[count] = strtol(element_buf, NULL, 0);
		pos += 2 * sizeof(char);
	}
}
