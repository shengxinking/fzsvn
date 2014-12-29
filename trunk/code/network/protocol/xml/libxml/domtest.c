/*
 *	@file	domtest.c
 *
 *	@brief	test libXML's DOM parser
 *	
 *	@date	2008-10-31
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libxml2/libxml/parser.h>

#ifndef NAMELEN
#define NAMELEN		63
#endif

static char _g_filename[NAMELEN + 1];
static char *_g_filebuf;
static int _g_filelen;

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("domtest <options>\n");
	printf("\t-f <file>\tthe XML file\n");
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
	char optstr[] = ":f:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'f':
			strncpy(_g_filename, optarg, NAMELEN);
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

	if (strlen(_g_filename) < 1)
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
	int fd;
	struct stat stat;

	fd = open(_g_filename, O_RDONLY);
	if (fd < 0) {
		printf("can't open file %s: %s\n", 
		       _g_filename, strerror(errno));
		return -1;
	}

	if (fstat(fd, &stat)) {
		printf("fstat file %s error: %s\n", 
		       _g_filename, strerror(errno));

		close(fd);
		return -1;
	}

	_g_filelen = stat.st_size;

	_g_filebuf = malloc(_g_filelen);
	if (!_g_filebuf) {
		printf("can't malloc memory: %s\n",
		       strerror(errno));

		close(fd);
		return -1;
	}

	if (read(fd, _g_filebuf, _g_filelen) != _g_filelen) {
		printf("read file %s error: %s\n", 
		       _g_filename, strerror(errno));
		
		close(fd);
		return -1;
	}

	close(fd);

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

static int 
_dom_parse(void)
{
	xmlDocPtr *doc;
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
	
	_dom_parse();

	_release();

	return 0;
}



