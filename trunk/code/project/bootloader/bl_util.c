/*
 *	@file	
 *
 *	@brief
 *	
 *	@date
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <getopt.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#define BL_PATHLEN	1024
#define BL_BUFLEN	1024

static char _g_infile[BL_PATHLEN];
static char _g_outfile[BL_PATHLEN];
static int  _g_first = 1;
static char _g_signature[2] = {0x55, 0xaa};

/**
 *	show usage of this program
 */
static void _usage(void)
{
	printf("bl_util: write first image or second image into device or device file\n");
	printf("usage: bl_util <options>\n");
	printf("\t-i\tthe input file\n");
	printf("\t-o\tthe output file\n");
	printf("\t-f\twrite first image to out file in 1st sector <default option>\n");
	printf("\t-s\twrite second image to out file from 2nd sector\n");
	printf("\t-h\tshow help message\n");
}

/**
  *	parse command line to get all options
  */
static int _parse_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":i:o:fsh";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'i':
			strncpy(_g_infile, optarg, BL_PATHLEN - 1);
			break;
		
		case 'o':
			strncpy(_g_outfile, optarg, BL_PATHLEN - 1);
			break;

		case 'f':
			_g_first = 1;
			break;

		case 's':
			_g_first = 0;
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

	if (strlen(_g_infile) < 1 || strlen(_g_outfile) < 1)
		return -1;

	if (argc != optind)
		return -1;

	return 0;
}

/**
  *	initiate the global variables and environment.
  */
static int _initiate(void)
{
	int fd;
	struct stat stat;

	if (access(_g_infile, F_OK)) {
		printf("can't read input file %s\n", _g_infile);
		return -1;
	}

	if (access(_g_outfile, F_OK)) {
		printf("can't access output file %s\n", _g_outfile);
		return -1;
	}

	return 0;
}

/**
  *	free global resource
  */
static int _release(void)
{
	return 0;
}


static int 
_write_first(int infd, int outfd)
{
	struct stat stat;
	u_int8_t buf[513] = {0};

	assert(infd >= 0);
	assert(outfd >= 0);

	if (fstat(infd, &stat)) {
		printf("stat file %s error: %s\n", 
		       _g_infile, strerror(errno));
		return -1;
	}

	if (stat.st_size != 512) {
		printf("first image file size is %d(correct is 512)\n",
		       stat.st_size);
		return -1;
	}

	if (read(infd, buf, 512) != 512) {
		printf("read file %s error: %s\n", 
		       _g_infile, strerror(errno));
		return -1;
	}

	if (buf[510] != 0x55 
	    || buf[511] != 0xaa) {
		printf("file %s signature error: 0x%x%x(correct is 0x55aa\n",
		       _g_infile, buf[510], buf[511]);
	}

	lseek(outfd, 0, SEEK_SET);
	
	/* we need skip Partition table area */
	if (write(outfd, buf, 446) != 446) {
		printf("write to device %s error: %s\n", 
		       _g_outfile, strerror(errno));
		return -1;
	}

	/* write signature to device */
	lseek(outfd, 510, SEEK_SET);
	write(outfd, _g_signature, 2);

	printf("write 512 bytes to %s\n", _g_outfile);

	return 0;
}

static int
_write_second(int infd, int outfd)
{
	char buf[512];
	int n;
	int m;
	int count = 0;

	assert(infd >= 0);
	assert(outfd >= 0);

	/* skip first sector in outfd */
	lseek(outfd, 512L, SEEK_SET);
	lseek(infd, 0L, SEEK_SET);

	n = read(infd, buf, 512);
	while (n > 0) {
		m = write(outfd, buf, n);
		if (m != n) {
			printf("write to device %s error: %s\n",
			       _g_outfile, strerror(errno));
			return -1;
		}
		count += m;

		n = read(infd, buf, 512);
	}

	if (n < 0) {
		printf("read from file %s error: %s\n",
		       _g_infile, strerror(errno));
		return -1;
	}

	printf("Write %d bytes to %s(1+)\n", 
	       count, _g_outfile);

	return 0;
}


/**
  *	read input file, and write them to output file.
  *	if write to first sector(MBR), avoid write patition table.
  */
static int _process(void)
{
	int infd, outfd;
	char buf[BL_BUFLEN];
	int ret = 0;

	infd = open(_g_infile, O_RDONLY);
	if (infd < 0) {
		printf("open input file %s error\n", _g_infile);
		return -1;
	}

	outfd = open(_g_outfile, O_RDWR);
	if (outfd < 0) {
		printf("open out file %s error\n", _g_outfile);
		return -1;
	}

	if (_g_first)
		ret = _write_first(infd, outfd);
	else
		ret = _write_second(infd, outfd);

	close(infd);
	close(outfd);

	return ret;
}


/**
  *	the main entry of program
  */
int main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	_process();

	printf("input file is %s, output file is %s\n", 
	       _g_infile, _g_outfile);

	_release();

	return 0;
}





