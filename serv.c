/* serv.cpp  -  Minimal ssleay server for Unix
 30.9.1996, Sampo Kellomaki <sampo@iki.fi> */

/* mangled to work with SSLeay-0.9.0b and OpenSSL 0.9.2b
 Simplified to be even more minimal
 12/98 - 4/99 Wade Scholine <wades@mail.cybg.com> */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/rsa.h>       /* SSLeay stuff */
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "tunproxy.c"

/* define HOME to be dir for key and cert files... */
#define HOME "./"
/* Make these what you want for cert & key files */
#define CERTF  HOME "server.crt"
#define KEYF  HOME  "server.key"
#define SHADOW_FILE_PATH  HOME  "shadow"
#define SERVER_PORT 1111
#define KEY_LEN 16
#define BUFFER_SIZE 4096
#define BUFFER_SIZE_SMALL 50
#define SEPARATOR ":"
#define SEPARATOR_LEN 1
#define HASH_LN 32
#define SALT_LN 16

#define CHK_NULL(x) if ((x)==NULL) exit (1)
#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(1); }
#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }



int main() {
	int err;
	int listen_sd;
	int sd;
	struct sockaddr_in sa_serv;
	struct sockaddr_in sa_cli;
	size_t client_len;
	SSL_CTX* ctx;
	SSL* ssl;
	char* str;
	char buf[BUFFER_SIZE];
	SSL_METHOD *meth;

	/* SSL preliminaries. We keep the certificate and key with the context. */

	SSL_load_error_strings();
	SSLeay_add_ssl_algorithms();
	meth = SSLv23_server_method();
	ctx = SSL_CTX_new(meth);
	if (!ctx) {
		ERR_print_errors_fp(stderr);
		exit(2);
	}

	if (SSL_CTX_use_certificate_file(ctx, CERTF, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(3);
	}
	if (SSL_CTX_use_PrivateKey_file(ctx, KEYF, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(4);
	}

	if (!SSL_CTX_check_private_key(ctx)) {
		fprintf(stderr,
				"Private key does not match the certificate public key\n");
		exit(5);
	}

	/* ----------------------------------------------- */
	/* Prepare TCP socket for receiving connections */

	listen_sd = socket(AF_INET, SOCK_STREAM, 0);
	CHK_ERR(listen_sd, "socket");

	memset(&sa_serv, '\0', sizeof(sa_serv));
	sa_serv.sin_family = AF_INET;
	sa_serv.sin_addr.s_addr = INADDR_ANY;
	sa_serv.sin_port = htons(SERVER_PORT);

	err = bind(listen_sd, (struct sockaddr*) &sa_serv, sizeof(sa_serv));
	CHK_ERR(err, "bind");

	/* Receive a TCP connection. */

	err = listen(listen_sd, 5);
	CHK_ERR(err, "listen");

	client_len = sizeof(sa_cli);
	sd = accept(listen_sd, (struct sockaddr*) &sa_cli, &client_len);
	CHK_ERR(sd, "accept");
	close(listen_sd);

	printf("Connection from %lx, port %x\n", sa_cli.sin_addr.s_addr,
			sa_cli.sin_port);

	/* ----------------------------------------------- */
	/* TCP connection is ready. Do server side SSL. */

	ssl = SSL_new(ctx);
	CHK_NULL(ssl);
	SSL_set_fd(ssl, sd);
	err = SSL_accept(ssl);
	CHK_SSL(err);

	/* DATA EXCHANGE - Receive message and send reply. */

	err = SSL_read(ssl, buf, sizeof(buf) - 1);
	CHK_SSL(err);

	// Extract the data from the received buffer
	unsigned char key[KEY_LEN];
	char userpass[BUFFER_SIZE_SMALL];
	int userpass_len = err - KEY_LEN;
	int index = 0;
	memcpy(&key[0], &buf[index], KEY_LEN);
	index += KEY_LEN;
	memcpy(&userpass[0], &buf[index], userpass_len);
	index += userpass_len;
	userpass[userpass_len] = '\0';
	memset(buf, 0, BUFFER_SIZE);



	char* message;
	// userpass contains username and password like this username:password
	int user_pass_verified = verifiy_user_pass(userpass, userpass_len);
	if (user_pass_verified == 0){
		message = "Wrong username or password.";
	} else {
		message = "Connection successful.";
	}
	printf("%s \n", message);

	int message_len = strlen(message);

	err = SSL_write(ssl, message, message_len);
	CHK_SSL(err);

	/* Clean up. */

	close(sd);
	SSL_free(ssl);
	SSL_CTX_free(ctx);

	if (user_pass_verified == 0){
		exit(1);
	}

	start(1, NULL, key);

	return 0;
}

int verifiy_user_pass(char userpass[], int userpass_len) {
	int result = 0;

	// Extract username and password from userpass
	int index_of_separator = find_char_in_string(userpass, SEPARATOR[0]);
	char username[BUFFER_SIZE_SMALL];
	int username_len = index_of_separator;
	char password[BUFFER_SIZE_SMALL];
	int password_len = userpass_len - username_len - SEPARATOR_LEN;
	int index = 0;
	memcpy(&username[0], &userpass[index], username_len);
	index += username_len;
	username[username_len] = '\0';
	index += SEPARATOR_LEN;
	memcpy(&password[0], &userpass[index], password_len);
	index += password_len;
	password[password_len] = '\0';


	// Open the shadow file that contains the login info
	FILE * shadow_file;
	char * username_shadow_string = NULL;
	ssize_t username_shadow_string_ln = 0;
	char * salt_shadow_string = NULL;
	ssize_t salt_shadow_string_ln = 0;
	char * hash_shadow_string = NULL;
	ssize_t hash_shadow_string_ln = 0;
	size_t len = 0;
	shadow_file = fopen(SHADOW_FILE_PATH, "r");
	if (shadow_file == NULL)
		exit(EXIT_FAILURE);

	// loop through the shadow file by reading 3 lines at a time (username, salt, hashed password)
	while ((username_shadow_string_ln = getline(&username_shadow_string, &len, shadow_file)) != -1) {
		salt_shadow_string_ln = getline(&salt_shadow_string, &len, shadow_file);
		hash_shadow_string_ln = getline(&hash_shadow_string, &len, shadow_file);
		username_shadow_string_ln--;
		salt_shadow_string_ln--;
		hash_shadow_string_ln--;
		username_shadow_string[username_shadow_string_ln] = '\0';


		// Convert hex strings to bytes array
		unsigned char salt_shadow_bytearray[SALT_LN];
		convert_hex_string_to_bytes_array (salt_shadow_string, salt_shadow_bytearray);
		unsigned char hash_shadow_bytearray[HASH_LN];
		convert_hex_string_to_bytes_array (hash_shadow_string, hash_shadow_bytearray);

		// Compare the hash that we calculate from password and salt with the hash we get from the shadow file
		unsigned char calculated_hash[HASH_LN];
		calculate_sha256_hash_with_salt(salt_shadow_bytearray, SALT_LN, password, password_len, calculated_hash, NULL);
		if (strcmp(username_shadow_string, username) == 0){
			if (compare_buffers(calculated_hash, hash_shadow_bytearray, HASH_LN) == 1){
				result = 1;
			}
			break;
		}
	}
	fclose(shadow_file);
	return result;
}