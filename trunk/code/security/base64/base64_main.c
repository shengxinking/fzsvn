/**
 *	@file	base64_main.c
 *
 *	@brief	Base64 encode, decode using BIO.
 *
 *	@author	Forrest.zhang
 *	
 *	@date	2009-11-21
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>

#include <openssl/bio.h>

#define	_NAME_LEN	127

static char _g_infile[_NAME_LEN + 1];
static char _g_outfile[_NAME_LEN + 1];
static int  _g_mode = 0;	/* 0 encode, 1 decode, 2 test */
static BIO  *_g_inbio = NULL;
static BIO  *_g_outbio = NULL;

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("base64 <options>\n");
	printf("\t-d\tdecode base64 data\n");
	printf("\t-e\tencode base64 data\n");
	printf("\t-t\ttest base64 encode/decode\n");
	printf("\t-i\tthe input file\n");
	printf("\t-o\tthe output file\n");
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
	char optstr[] = ":deti:o:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'd':
			_g_mode = 1;
			break;

		case 'e':
			_g_mode = 0;
			break;

		case 't':
			_g_mode = 2;
			break;

		case 'i':
			strncpy(_g_infile, optarg, _NAME_LEN);
			break;

		case 'o':
			strncpy(_g_outfile, optarg, _NAME_LEN);
			break;

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

	if (strlen(_g_infile) < 1) {
		printf("no input file\n");
		return -1;
	}

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
	_g_inbio = BIO_new_file(_g_infile, "r");
	if (!_g_inbio) {
		printf("open input file %s failed\n", _g_infile);
		return -1;
	}

	if (strlen(_g_outfile) > 0) {
		_g_outbio = BIO_new_file(_g_outfile, "wb");
		if (!_g_outbio) {
			printf("open out file %s failed\n", _g_outfile);
			return -1;
		}
	}
	else {
		_g_outbio = BIO_new_fp(stdout, BIO_NOCLOSE);
		if (!_g_outbio) {
			printf("using stdout as out BIO failed\n");
			return -1;
		}
	}

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
	if (_g_inbio)
		BIO_free(_g_inbio);
	
	if (_g_outbio) 
		BIO_free(_g_outbio);
}


static int 
_base64_encode(BIO *in, BIO *out)
{
	return 0;
}

static int 
_base64_decode(BIO *in, BIO *out)
{
	return 0;
}


static int 
_base64_test(BIO *in)
{
	BIO *b64;
	BIO *ein;
	BIO *din;
	BIO *mem;
	char buf[1024];
	int n = 0;

	printf("the input:\n");
	memset(buf, 0, sizeof(buf));
	while ( (n = BIO_read(in, buf, 1023)) > 0) {
		printf("%s", buf);
		memset(buf, 0, sizeof(buf));
	}
	printf("\n\n");
	BIO_reset(in);

	b64 = BIO_new(BIO_f_base64());
	mem = BIO_new(BIO_s_mem());

	ein = BIO_push(b64, mem);
	printf("ein %p, b64 %p\n", b64, ein);

	while ( (n = BIO_read(in, buf, 1023)) > 0) {
		BIO_write(ein, buf, n);
	}
	BIO_reset(in);
	
	printf("the encode:\n");
	while ( (n = BIO_read(mem, buf, 1023)) > 0) {
		printf("%s", buf);
		memset(buf, 0, sizeof(buf));
	}
	printf("\n\n");

	

	return 0;
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

	if (_g_mode == 0) {
		_base64_encode(_g_inbio, _g_oubio);
	}
	else if(_g_mode == 1) {
		_base64_decode(_g_inbio, _g_outbio);
	}
	else if (_g_mode == 2) {
		_base64_test(_g_inbio);
	}

	_release();

	return 0;
}



