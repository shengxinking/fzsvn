/**
 *	@file	
 *
 *	@brief
 *
 *	@author	Forrest.zhang
 *	
 *	@date
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <zlib.h>
#include <fcntl.h>

enum {
	GZIP_COMPRESS,
	GZIP_UNCOMPRESS,
	GZIP_TEST,
};

static int	_g_type;
static char	_g_infile[64];
static char	_g_outfile[64];

static char	_g_optstr[] = ":cuti:o:h";


/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("<gzip_test <options>");
	printf("\t-c\tcompress file\n");
	printf("\t-u\tuncompress file\n");
	printf("\t-t\tcheck the compress file\n");
	printf("\t-i\tinput file name\n");
	printf("\t-o\toutput file name\n");
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
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, _g_optstr)) != -1) {
		
		switch (opt) {
			
		case 'c':
			_g_type = GZIP_COMPRESS;
			break;
		case 'u':
			_g_type = GZIP_UNCOMPRESS;
			break;
		case 't':
			_g_type = GZIP_TEST;
			break;
		case 'i':
			strncpy(_g_infile, optarg, sizeof(_g_infile) - 1);
			break;
		case 'o':
			strncpy(_g_outfile, optarg, sizeof(_g_outfile) - 1);
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

	if (strlen(_g_infile) < 1) {
		printf("no input file\n");
		return -1;
	}
	
	if (_g_type != GZIP_TEST && strlen(_g_outfile) < 1) {
		printf("no output file\n");
		return -1;
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
	gzFile gzin = NULL, gzout = NULL;
	int infd, outfd;
	char buf[1024];
	int n, m;
//	int errnum = Z_OK;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	switch (_g_type) {
	case GZIP_COMPRESS:
		infd = open(_g_infile, O_RDONLY);
		if (infd < 0) {
			printf("open input file %s failed\n", _g_infile);
			break;
		}
		gzout = gzopen(_g_outfile, "w");
		if (gzout == Z_NULL) {
			printf("open output file %s failed\n", _g_infile);
			close(infd);
			break;
		}
		do {
			n = read(infd, buf, 1024);
			if (n < 0) {
				printf("read input file %s failed: %s\n\n", 
				       _g_infile, strerror(errno));
				break;
			}
			if (n == 0) {
				break;
			}
			m = gzwrite(gzout, buf, n);
			if (m != n) {
				printf("write to output file %s failed: %s\n",
				       _g_outfile, gzerror(gzout, NULL));
				break;
			}
		} while (n > 0);
		close(infd);
		gzclose(gzout);
		break;
	case GZIP_UNCOMPRESS:
		gzin = gzopen(_g_infile, "r");
		if (gzin == Z_NULL) {
			printf("open input file %s failed\n", _g_infile);
			break;
		}
		if (gzdirect(gzin)) {
			printf("input file %s is not compress file\n", _g_infile);
			gzclose(gzin);
			break;
		}
		
		outfd = open(_g_outfile, O_WRONLY | O_CREAT, 0644);
		if (outfd < 0) {
			printf("open output file %s failed\n", _g_outfile);
			gzclose(gzin);
			break;
		}	
		do {
			n = gzread(gzin, buf, 1024);
			if (n < 0) {
				printf("read input file %s failed(%d): %s\n\n", 
				       _g_infile, n, gzerror(gzin, NULL));
				break;
			}
			if (n == 0) {
				if (gzeof(gzin))
					break;
				printf("read input file %s failed(%d): %s\n\n", 
				       _g_infile, n, gzerror(gzin, NULL));
				break;
			}
			m = write(outfd, buf, n);
			if (m != n) {
				printf("write to output file %s failed: %s\n",
				       _g_outfile, strerror(errno));
				break;
			}
		} while (n > 0);
		gzclose(gzin);
		close(outfd);
		break;
	case GZIP_TEST:
		gzin = gzopen(_g_infile, "r");
		if (gzin == Z_NULL) {
			printf("open input file %s failed: %s\n", 
			       _g_infile, strerror(errno));
			break;
		}
		if (gzdirect(gzin)) {
			printf("input file %s is not compress file\n", _g_infile);
			gzclose(gzin);
			break;
		}
		do {
			n = gzread(gzin, buf, 1024);
			if (n < 0) {
				printf("gzread %s failed(%d): %s\n", 
				       _g_infile, n, gzerror(gzin, NULL));
				break;
			}
			else if (n == 0) {
				if (!gzeof(gzin) ) {
					printf("gzread %s failed(%d): %s\n", 
					       _g_infile, n, gzerror(gzin, NULL));
				}
				break;
			}
		} while (n > 0);
		if (gzclose(gzin) != Z_OK) {
			printf("gzclose failed\n");
		}
		break;
	}

	_release();

	return 0;
}



