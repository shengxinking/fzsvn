/**
 *	@file	digest_test.c
 *
 *	@brief	using EVP functions to do digest
 *
 *	@author	Forrest.zhang
 *	
 *	@date	2009-09-15
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>

#include <openssl/evp.h>

static char _g_infile[BUFLEN];	/* the input file */
static char _g_string[BUFLEN];	/* the string need digest */
static int _g_verbos;		/* show verbos message */
static int _g_digest;		/* the digest method */
static char *_g_inbuf;		/* the digest input buffer */


/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("digest_test <options>\n\n");
	printf("\t-f\tthe file need digest\n");
	printf("\t-s\tthe string need digest\n");
	printf("\t-m\tthe digest method: md2, md4, md5, sha1\n");
	printf("\t-v\tshow verbose information\n");
	printf("\t-h\tshow help message\n");
}

/**
 *	Parse command line argument.	
 *
 * 	Return 0 if parse success, -1 on error.
 */
static int 
_parse_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":f:s:m:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'h':
			return -1;

		case ':':
			printf("Option %c missing argument\n", optopt);
			return -1;

		case '?':
			printf("Unknowed option %c\n", optopt);
			return -1;
		}
	}

	if (argc != optind)
		return -1;

	return 0;
}

/**
 *	Init some global resource used in program.	
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_initiate(void)
{
	/* add all algorithms */
	OpenSSL_add_all_algorithms();
	return 0;
}


/**
 *	Release global resource alloced by _initiate().	
 *
 * 	No Return.
 */
static void 
_release(void)
{
}


/**
 *	The main entry of program.	
 *
 * 	Return 0 if success, other value on error.
 */
int 
main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	_release();

	return 0;
}



