/*
 *	@file	pcreperf_test.c
 *
 *	@brief	Test pcre performance using Intel C compiler and GCC compiler
 *	
 *	@date	2008-10-23
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pcre.h>

#define RESTR_MAX	4095
#define PCRE_MAX	100
#define PCRE_MATCH_CNT	30
#define	THREAD_MAX	20
#define BUFF_MAX	127

typedef struct pcre_match {
	char		restr[RESTR_MAX + 1];
	pcre		*pcre;
	int		nmatche;
	pthread_mutex_t	lock;
} pcre_match_t;


static pcre_match_t	_g_matches[PCRE_MAX];
static int		_g_nmatche = 0;
static char		*_g_buf;
static int		_g_buflen = 0;
static char		_g_pcrefile[BUFF_MAX + 1];
static char		_g_file[BUFF_MAX + 1];
static int		_g_count = 1;
static int		_g_nworks = 1;

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("Usage: pcreperf_test [options] file\n");
	printf("\t-r\tthe regular expression, you can put more than one\n");
	printf("\t-R\tthe regular expression file, each line a PCRE line");
	printf("\t-f\tthe file run PCRE matches\n");
	printf("\t-c\tthe loop count\n");
	printf("\t-n\nthe work number\n");
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
	char optstr[] = ":r:R:f:c:n:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'r':
			if (_g_nmatche > PCRE_MAX) {
				printf("No more than %d regular expression\n",
					PCRE_MAX);
				return -1;
			}
			strncpy(_g_matches[_g_nmatche].restr, optarg, RESTR_MAX);
			_g_nmatche++;
			break;

		case 'R':
			strncpy(_g_pcrefile, optarg, BUFF_MAX);
			break;

		case 'c':
			_g_count = atoi(optarg);
			if (_g_count < 1)
				return -1;
			break;

		case 'f':
			strncpy(_g_file, optarg, BUFF_MAX);
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

	if (_g_nmatche == 0)
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
	int left, m, i;
	char *ptr;
	struct stat st;
	const char *error = NULL;
	int erroff = 0;

	if (stat(_g_file, &st))
		return -1;

	_g_buflen = st.st_size;
	_g_buf = malloc(_g_buflen);
	if (!_g_buf)
		return -1;
	
	/* read file to memory */
	fd = open(_g_file, O_RDONLY);
	if (fd < 0)
		return -1;

	left = _g_buflen;
	ptr = _g_buf;
	while (left) {
		m = read(fd, ptr, left);
		if (m <= 0)
			return -1;
		
		left -= m;
		ptr += m;
	}
	printf("read file %s %ld bytes\n", 
	       _g_file, st.st_size);

	for (i = 0; i < _g_nmatche; i++) {
		_g_matches[i].pcre = pcre_compile(_g_matches[i].restr, 
						  PCRE_UTF8, 
						  &error, &erroff, 
						  NULL);
		if (_g_matches[i].pcre == NULL) {
			printf("PCRE compile %s error at pos %d: %s\n", 
			       _g_matches[i].restr, erroff, error);
			return -1;
		}
		printf("PCRE compile: %s\n", _g_matches[i].restr);
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
	int i;

	/* free file buffer */
	if (_g_buf)
		free(_g_buf);

	/* free PCRE */
	for (i = 0; i < _g_nmatche; i++) {
		if (_g_matches[i].pcre) {
			pcre_free(_g_matches[i].pcre);
			_g_matches[i].pcre = NULL;
		}
	}
}


static int 
_pcre_parse()
{
	int i, j;
	int match_pos = 0;
	int res = 0;
	int matches[PCRE_MATCH_CNT];
	struct timeval begin, end;
	long sec;
	long usec;
	char *matchstr = NULL;

	gettimeofday(&begin, NULL);

	for (i = 0; i < _g_count; i++) {
		for (j = 0; j < _g_nmatche; j++) {
			match_pos = 0;
			do {
				res = pcre_exec(_g_matches[j].pcre, NULL, _g_buf, 
						_g_buflen, match_pos, 0, matches, PCRE_MATCH_CNT);

//				printf("res is %d\n", res);
				if (res <= 0) {
					if (res != PCRE_ERROR_NOMATCH)
						printf("PCRE match error\n");

					break;
				}

				if (pcre_get_substring(_g_buf, matches, res, 0, &matchstr) > 0) {

//					printf("the match string is(%d %d %d): %s\n", 
//						matches[0], matches[1], matches[2], matchstr);
				}

				pcre_free_substring(matchstr);

				match_pos = matches[1];
				
				_g_matches[j].nmatche++;

			} while (res > 0);
		}
	}
	gettimeofday(&end, NULL);

	if (end.tv_usec < begin.tv_usec) {
		usec = 1000000 + end.tv_usec - begin.tv_usec;
		end.tv_sec --;
	}
	else
		usec = end.tv_usec - begin.tv_usec;
	sec = end.tv_sec - begin.tv_sec;

	printf("PCRE match results:\n");
	for (i = 0; i < _g_nmatche; i++) {
		printf("%2d: %s matched %d times\n",
		       i, _g_matches[i].restr, _g_matches[i].nmatche);
	}
	printf("\n");
	printf("PCRE parse file %s using %d regular expressions %d times\n",
	       _g_file, _g_nmatche, _g_count);
	printf("PCRE spend %ld second %ld usecond\n", sec, usec);

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

	_pcre_parse();

	_release();

	return 0;
}



