#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 4096

int argv_cnt = 1;
int encoding = -1;
int salted = 1;
char password[256];
char input[256];
char output[256];
unsigned char buf[BUF_SIZE];
unsigned char cipherbuf[BUF_SIZE + 16];

void print_help(char **argv)
{
	printf("use: %s -e|-d -p password -in input -out output -nosalt (optional)\n", argv[0]);
	exit(1);
}


void handle_option(int argc, char **argv)
{

	switch(argv[argv_cnt][0]){
		case '-':

			if (strcmp(argv[argv_cnt]+1,"e") == 0 ){
				printf(" encoding = 1 \n");
                encoding = 1;
			}
			else if (strcmp(argv[argv_cnt]+1,"d") == 0 ){
				printf(" encoding = 0 \n");
                encoding = 0;
			}
			else if (strcmp(argv[argv_cnt]+1,"p") == 0 ){
				argv_cnt++;
				if ( argv_cnt == argc )
					print_help(argv);
				printf(" password = %s \n", argv[argv_cnt]);
                strcpy(password, argv[argv_cnt]);
			}
			else if (strcmp(argv[argv_cnt]+1,"in") == 0 ){
				argv_cnt++;
				if ( argv_cnt == argc ){
					printf("no input is specified\n");
				 	print_help(argv);
				}
				printf(" input = %s \n", argv[argv_cnt]);
                memcpy(input, argv[argv_cnt], strlen(argv[argv_cnt]));
			}
			else if (strcmp(argv[argv_cnt]+1,"out") == 0 ){
				argv_cnt++;
				if ( argv_cnt == argc )
					print_help(argv);
				printf(" output = %s \n", argv[argv_cnt]);
                memcpy(output, argv[argv_cnt], strlen(argv[argv_cnt]));
			}
            else if (strcmp(argv[argv_cnt]+1,"nosalt") == 0 ){
				printf(" nosalt used\n");
                salted = 0;
			}
			else if (strcmp(argv[argv_cnt]+1,"help") == 0 ){
				print_help(argv);
			}
			else {
				printf("%s: unknown option\n", argv[argv_cnt]);
				print_help(argv);
			}

			argv_cnt++;
			break;

		default:
			printf("%s: unknown option\n", argv[argv_cnt]);
			print_help(argv);
			break;
	}

	return;

}

void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

// portions of the code are adapted from the wesbite mentioned in my logs especially encrypt and decrypt
int encrypt( int inputFd, int outputFd, unsigned char *key, unsigned char *iv)
{
    EVP_CIPHER_CTX *ctx;

    int len;
    int ciphertext_len = 0;
    int numRead;

    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    while ((numRead = read(inputFd, buf, BUF_SIZE)) > 0) {
        if(1 != EVP_EncryptUpdate(ctx, cipherbuf, &len, buf, numRead)) {
            handleErrors();
        }
        if (write(outputFd, cipherbuf, len) != len) {
            printf("write() returned error or partial write occurred");
            return 1;
        }
        ciphertext_len += len;
    }

    if (numRead == -1) {
        printf("read error\n");
        return 1;
    }

    if(1 != EVP_EncryptFinal_ex(ctx, cipherbuf, &len))
        handleErrors();

    if (write(outputFd, cipherbuf, len) != len)
        handleErrors();

    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}


// removed unsigned char *ciphertext, int ciphertext_len, unsigned char *plaintext

int decrypt(int inputFd, int outputFd, unsigned char *key, unsigned char *iv)
{
    EVP_CIPHER_CTX *ctx;

    int len;
    int plaintext_len = 0;
    ssize_t numRead;

    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    while ((numRead = read(inputFd, cipherbuf, BUF_SIZE )) > 0) {
        if(1 != EVP_DecryptUpdate(ctx, buf, &len, cipherbuf, numRead)) {
            handleErrors();
        }
        if (write(outputFd, buf, len) != len) {
            printf("write() returned error or partial write occurred");
            return 1;
        }
        plaintext_len += len;
    }

    if (numRead == -1) {
        printf("read error\n");
        return 1;
    }

    if(1 != EVP_DecryptFinal_ex(ctx, buf, &len))
        handleErrors();

    if (write(outputFd, buf, len) != len)
        handleErrors();

    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}


int main(int argc, char **argv)
{
	if ( argc == 1 ) {
		print_help(argv);
    }

	while( argv_cnt < argc ) {
		handle_option(argc, argv);
    }

    int inputFd, outputFd, openFlags;
    mode_t filePerms;
    
    /* Open input and output files */

    inputFd = open(input, O_RDONLY);
    if (inputFd == -1) {
        printf("error opening file %s\n", input);
        return 1;
    }

    openFlags = O_CREAT | O_WRONLY | O_TRUNC;
    filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;      /* rw-rw-rw- */

    outputFd = open(output, openFlags, filePerms);
    if (outputFd == -1) {
        printf("opening file %s\n", output);
        return 1;
    }

    unsigned char key[32];
    unsigned char iv[16];
    unsigned char *salt = malloc(8);

    if (encoding == 1) {
        printf("encrypting\n");

        if (salted == 1) {
            RAND_bytes(salt, 8); // generate salt

            if (write(outputFd, "Salted__", 8) != 8) {
                printf("Error writing Salted__\n");
                return 1;
            }
            if (write(outputFd, salt, 8) != 8) {
                printf("Error writing salt\n");
                return 1;
            }
        }
        else {
            free(salt);
            salt = NULL;
        }

        EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), salt, (unsigned char*)password, strlen(password), 1, key, iv);

        encrypt(inputFd, outputFd, key, iv);
    }
    else if (encoding == 0) {
        printf("decrypting\n");

        if (salted == 1) {
            lseek(inputFd, 8, SEEK_SET); // seek to salt
            read(inputFd, salt, 8);
        }
        else {
            free(salt);
            salt = NULL;
        }

        EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), salt, (unsigned char*)password, strlen(password), 1, key, iv);

        decrypt(inputFd, outputFd, key, iv);
    }

    //close the files
    if (close(inputFd) == -1) {
        printf("close input error\n");
        return 1;
    }
    if (close(outputFd) == -1) {
        printf("close output error\n");
        return 1;
    }

    free(salt);

    return 0;
}