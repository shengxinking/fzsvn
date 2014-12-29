/*
 *
 *
 *
 */

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/engine.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>


#include "keyfile.h"

/*
 *
 *
 */
int keyfile_rsa_bits(const char *file)
{
	RSA *rsa = NULL;
	FILE *fp = NULL;
	int bits = -1;

	if (access(file, R_OK)) {
		return -1;
	}

	fp = fopen(file, "r");
	if (!fp) {
		return -1;
	}

	rsa = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);
	if (!rsa) {
		fclose(fp);
		return -1;
	}
	if (!rsa->n) {
		fclose(fp);
		RSA_free(rsa);
		return -1;
	}

	bits = BN_num_bits(rsa->n);

	RSA_free(rsa);
	fclose(fp);

	return bits;
}

/*
 *
 *
 *
 */
int keyfile_dsa_bits(const char *keyfile)
{
	return -1;
}


/*
 *
 *
 */
int keyfile_type(const char *keyfile)
{
	char buf[1024] = {0};
	FILE *fp = NULL;
	int ret = KEYFILE_NONE;

	fp = fopen(keyfile, "r");
	if (!fp)
		return KEYFILE_NONE;

	fgets(buf, 1023, fp);

	if (strncmp(buf, "-----BEGIN PUBLIC KEY-----", 26) == 0) {
	        ret = KEYFILE_PUBLIC;
	}
	else if (strncmp(buf, "-----BEGIN RSA PRIVATE KEY-----", 30) == 0) {
		ret = KEYFILE_RSA_PRIVATE;
	}
	else if (strncmp(buf, "-----BEGIN DSA PRIVATE KEY-----", 31) == 0) {
		ret = KEYFILE_DSA_PRIVATE;
	}

	fclose(fp);

	return ret;
}
