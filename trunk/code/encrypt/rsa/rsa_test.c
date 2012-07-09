/*
 * 
 *
 *
 *
 *
 */

#include <openssl/engine.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "keyfile.h"

static const char *g_keyfile = NULL;
static const char *g_passwd = NULL;

static void _usage(void)
{
	printf("rsa_test <rsa key file> [password for keyfile]\n");
}

static int _parse_cmd(int argc, char **argv)
{
	if (argc != 2 && argc != 3)
		return -1;

	if (access(argv[1], R_OK)) {
		printf("can't read file %s\n", argv[1]);
		return -1;
	}

	g_keyfile = argv[1];

	if (argc == 3) {
		g_passwd = argv[2];
	}

	return 0;
}

static int _init(void)
{
	return 0;
}

static void _release(void)
{
	return;
}

int _pass_cb(char *buf, int size, int rwflag, void *u)
{
	int len = 0;
	char *str;

	printf("1\n");

	str = u;
	if (!str)
		return 0;

	printf("passwd is %s\n", str);
	
	len = strlen(str);
	if (len > size)
		len = size;

	memcpy(buf, str, len);
	return len;
} 


int main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_init()) {
		_release();
		return -1;
	}

	printf("%s type is %d\n", g_keyfile, keyfile_type(g_keyfile));
	

	printf("%s modulu size is %d\n", 
	       g_keyfile, keyfile_rsa_bits(g_keyfile));


	return 0;
}





