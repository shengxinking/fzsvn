/*
 *	@file	
 *
 *	@brief
 *	
 *	@date
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

#include <openssl/sha.h>

#define _FILELEN	31
#define _BUFSIZ		64
static char _g_infile[_FILELEN + 1];
static char _g_outfile[_FILELEN + 1];
static int _g_infd = -1;
static int _g_outfd = -1;
static SHA_CTX _g_sha1ctx;

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("sha1 <options>\n");
	printf("\t-i\tinput file\n");
	printf("\t-o\toutput file, if not set, show in stdout\n");
	printf("\t-h\tshow help information\n");
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
	char optstr[] = ":i:o:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'i':
			strncpy(_g_infile, optarg, _FILELEN);
			break;

		case 'o':
			strncpy(_g_outfile, optarg, _FILELEN);
			break;

		case 'h':
			_usage();
			exit(0);

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

	if (strlen(_g_infile) < 1)
		return -1;

	if (access(_g_infile, R_OK)) {
		printf("can't read input file %s\n", _g_infile);
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
	_g_infd = open(_g_infile, O_RDONLY);
	if (_g_infd < 0) {
		printf("open input file %s error: %s\n",
		       _g_infile, strerror(errno));
		return -1;
	}

	if (strlen(_g_outfile) > 0) {
		_g_outfd = open(_g_outfile, O_WRONLY | O_CREAT, 0644);
		if (_g_outfd < 0) {
			printf("open output file %s error: %s\n",
			       _g_outfile, strerror(errno));
			return -1;
		}
	}
	else
		_g_outfd = -1;
	
	SHA1_Init(&_g_sha1ctx);

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
	if (_g_infd > -1)
		close(_g_infd);

	if (_g_outfd > -1)
		close(_g_outfd);
}


static int _do_loop(void)
{
	unsigned char buf[_BUFSIZ];
	unsigned char digest[SHA_DIGEST_LENGTH + 1];
	int n = 0;
	
	do {
		n = read(_g_infd, buf, _BUFSIZ);
		if (n > 0)
			SHA1_Update(&_g_sha1ctx, buf, n);
	} while (n > 0);

	SHA1_Final(digest, &_g_sha1ctx);
	digest[SHA_DIGEST_LENGTH] = 0;

	if (_g_outfd >= 0) {
		write(_g_outfd, digest, SHA_DIGEST_LENGTH);
		close(_g_outfd);
		_g_outfd = -1;
	}
	else {
		int i;
		printf("SHA1(%s)= ", _g_infile);
		for (i = 0; i < SHA_DIGEST_LENGTH; i++)
			printf("%.2x", digest[i]);
		printf("\n");
	}

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

	_do_loop();

	_release();

	return 0;
}



