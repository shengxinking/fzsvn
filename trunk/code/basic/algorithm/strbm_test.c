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
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include "strbm.h"

static char	*buffer = NULL;
static char	*filename = NULL;
static char	*pattern = NULL;
static int	patlen = 0;
static int	buflen = 0;
static int	*badchar = NULL;
static int	*goodsuffix = NULL;


/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("strbm_test <option>\n");
	printf("\t-t\tthe text string need search\n");
	printf("\t-f\tthe text file need search\n");
	printf("\t-p\tthe pattern for search\n");
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
	char optstr[] = ":t:f:p:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			

		case 't':
			buflen = strlen(optarg);
			if (buffer)
				free(buffer);
			buffer = malloc(buflen + 1);
			if (!buffer)
				return -1;

			memcpy(buffer, optarg, buflen);
			buffer[buflen] = 0;

			break;

		case 'f':
			filename = optarg;
			break;

		case 'p':
			patlen = strlen(optarg);
			pattern = malloc(patlen + 1);
			if (!pattern)
				return -1;

			memcpy(pattern, optarg, patlen);
			pattern[patlen] = 0;
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
	struct stat st;

	if (filename) {
		if (buffer)
			free(buffer);

		if (stat(filename, &st)) {
			printf("stat file %s failed: %s\n", 
			       filename, strerror(errno));
			return -1;
		}
		
		buflen = st.st_size;

		fd = open(filename, O_RDONLY);
		if (fd < 0) {
			printf("open file %s failed: %s\n", 
			       filename, strerror(errno));
			return -1;
		}

		buffer = malloc(buflen + 1);
		if (read(fd, buffer, buflen) != buflen) {
			printf("read file %s failed %s\n",
			       filename, strerror(errno));
			free(buffer);
			close(fd);
			return -1;
		}

		buffer[buflen] = 0;
	}

	if (!buffer || !pattern)
		return -1;

	badchar = malloc(sizeof(int) * 256);
	if (!badchar) {
		return -1;
	}

	goodsuffix = malloc(sizeof(int) * patlen);
	if (!goodsuffix) {
		return -1;
	}

	return 0;
}


static char *
my_strstr(char *buffer, char *pattern)
{
	char *ptr1;
	char *ptr2;
	char *ptr3;

	ptr1 = buffer;
	while (*ptr1) {
		ptr2 = ptr1;
		ptr3 = pattern;

		while (*ptr2++ == *ptr3++ && *ptr1 != 0 && *ptr2 != 0);
		
		if (*ptr3 == 0)
			return ptr1;

		ptr1++;
	}

	return NULL;
}


/**
 *	Release global resource alloced by _initiate().	
 *
 * 	No Return.
 */
static void 
_release(void)
{
	if (buffer)
		free(buffer);

	if (pattern)
		free(pattern);
}


/**
 *	The main entry of program.	
 *
 * 	Return 0 if success, other value on error.
 */
int 
main(int argc, char **argv)
{
	char *ptr = NULL;
	struct timeval tv1, tv2;
	unsigned long nsec, nusec;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	strbm_bc(pattern, patlen, badchar, 256);

	strbm_gs(pattern, patlen, goodsuffix, patlen);

	gettimeofday(&tv1, NULL);

	ptr = strbm_search(buffer, buflen, pattern, patlen, 
			   badchar, 256, goodsuffix, patlen);

	gettimeofday(&tv2, NULL);

	if (ptr) {
		printf("BM find\n");
	}

	if (tv2.tv_usec < tv1.tv_usec) {
		tv2.tv_usec += 1000000;
		tv2.tv_sec--;
	}

	nusec = tv2.tv_usec - tv1.tv_usec;
	nsec = tv2.tv_sec - tv1.tv_sec;

	printf("BM: spend %lu second %lu usecond\n", nsec, nusec);


	/* strstr version */

	gettimeofday(&tv1, NULL);

	ptr = my_strstr(buffer, pattern);

	gettimeofday(&tv2, NULL);

	if (ptr) {
		printf("strstr find: \n");
	}

	if (tv2.tv_usec < tv1.tv_usec) {
		tv2.tv_usec += 1000000;
		tv2.tv_sec--;
	}

	nusec = tv2.tv_usec - tv1.tv_usec;
	nsec = tv2.tv_sec - tv1.tv_sec;

	printf("strstr: spend %lu second %lu usecond\n", nsec, nusec);

	_release();

	return 0;
}



