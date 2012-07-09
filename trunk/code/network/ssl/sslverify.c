/*
 * @file	verify.c
 * @brief	verify a CA file and the private key associate it
 * 
 * @author	Forrest.zhang
 */

#include <string.h>
#include <unistd.h>

#include "sslcommon.h"

static char	*_g_cafile = NULL;
static char	*_g_keyfile = NULL;
static char	*_g_passwd = NULL;

static void 
_usage(void)
{
	printf("verify <CA file> <Private key file> <passwd>\n");
}

static int 
_parse_cmd(int argc, char **argv)
{
	if (argc != 4)
		return -1;

	_g_cafile = argv[1];
	if (access(_g_cafile, R_OK)) {
		printf("can't access CA file %s\n", _g_cafile);
		return -1;
	}
	
	_g_keyfile = argv[2];
	if (access(_g_keyfile, R_OK)) {
		printf("can't access Private key file %s\n", _g_keyfile);
		return -1;
	}

	_g_passwd = argv[3];

	return 0;
}

static int 
_initiate(void)
{
	return 0;
}

static void 
_release(void)
{
}

int 
main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate())
		return -1;

	ssl_cert_verify(_g_cafile, _g_keyfile, _g_passwd);

	_release();

	return 0;
}


