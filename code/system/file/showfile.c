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
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>

static int _g_offset = 3; /* offset format, 0 is OCT, 1 is DEC, 2 is HEX */
static int _g_width = 16; /* BYTES in one line, 16 is default */
static int _g_skip = 0;   /* skipe BYTES in first of input file */
static int _g_dump = 0;   /* dump BYTES in input file, 0 is dump until EOF */
static int _g_ftype = 0;  /* format data output, 0 is CHAR, 1 is OCT, 2 is DEC, 3 is HEX */
static int _g_fnum = 1;   /* each unit length, 1, 2, 4 bytes */
static char _g_file[64];  /* input file */

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("Usage: showfile [OPTION] ... [FILE]\n\n");
	printf("Write an unambiguous representation, octal bytes by \n");
	printf("defaults, of FILE to standard output. The data can show\n");
	printf("as Octal, Decimal,\nHexadecimal.\n\n");
	printf("All arguments are short options.\n");
	printf("\t-A\tdecide how file offsets are print, default is Hex\n");
	printf("\t-j\tskip BYTES input bytes first\n");
	printf("\t-N\tlimit dump to BYTES input bytes\n");
	printf("\t-t\tselect output format or formats\n");
//	printf("\t-w\toutput BYTES bytes prer output line, default is 16\n");
	printf("\t-h\tshow help message\n\n");
	printf("the data format support c|x|Xo|O|d|D[N] for HEX, N means\n");
	printf("bytes, only support 1, 2, 4, lower-case format char \n");
	printf("without width means 1 bytes data, for example, x means\n");
	printf("x1 or X1, o means o1 or O1. the offset format is\n");
	printf("o|O|x|X|d|D for OCT, HEX, DEC format\n");
}

/**
 *	Decide the format type or offset address type.
 *
 *	Return 0(OCT), 1(DEC), 2(HEX) if success, -1 on error.
 */
static int 
_get_type(char c) {

	switch (c) {

	case 'c':
		return 0;

	case 'o': case 'O':
		return 1;

	case 'd': case 'D':
		return 2;

	case 'x': case 'X':
		return 3;
	}

	return -1;
}

/**
 *	Decide data unit width.
 */
static int 
_get_num(const char *str, char type)
{
	if (strlen(str) > 1)
		return -1;
	
	if (strlen(str) == 0) {
		switch (type) {
		case 'c': case 'o': case 'x': case 'd':
			return 1;

		case 'O': case 'X': case 'D':
			return 2;
		}
		return -1;
	}

	if (type == 'c' && strlen(str) > 0)
		return -1;

	switch (str[0]) {
	case '1':
		return 1;

	case '2':
		return 2;

	case '4':
		return 4;
	}

	return -1;
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
	char optstr[] = ":A:j:N:t:w:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'A':
			_g_offset = _get_type(optarg[0]);
			if (_g_offset < 0 || strlen(optarg) != 1) {
				printf("invalid offset format, need x|X|o|O|d|D\n");
				return -1;
			}
			break;

		case 'j':
			_g_skip = atoi(optarg);
			if (_g_skip < 0) {
				printf("invalid skip BYTES\n");
				return -1;
			}
			break;

		case 'N':
			_g_dump = atoi(optarg);
			if (_g_dump < 0) {
				printf("invalid dump BYTES\n");
				return -1;
			}
			break;

		case 't':
			_g_ftype = _get_type(optarg[0]);
			_g_fnum = _get_num(optarg + 1, optarg[0]);
			if (_g_ftype < 0 || _g_fnum < 0) {
				printf("invalid format\n");
				return -1;
			}
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

	if (argc != optind + 1) {
		printf("need provide input file\n");
		return -1;
	}

	strcpy(_g_file, argv[optind]);
	if (access(_g_file, R_OK)) {
		printf("can't open file %s\n", _g_file);
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

static void 
_print_line_char(const char *buf, int num)
{
	int i;
	unsigned char c;
	
	for (i = 0; i < num; i++) {
		c = (unsigned char)buf[i];
		
		if (isascii(c) && isgraph(c))
			printf(" %c ", c);
		else
			printf("%02x ", c);
	
	}
}

static void 
_print_line_byte(const char *buf, int num)
{
	int i;
	unsigned char c;

	for (i = 0; i < num; i++) {

		c = (unsigned char)buf[i];
		switch (_g_ftype) {

		case 1:
			printf("%03o ", c);
			break;

		case 2:
			printf("%03d ", c);
			break;

		case 3:
			printf("%02x ", c);
			break;
		}
	}
}

static void 
_print_line_short(const char *buf, int num)
{
	int i;
	unsigned short val;

	for (i = 0; i < num; i += 2) {

		val = *(unsigned short *)(buf + i);
		switch (_g_ftype) {

		case 1:
			printf("%06o ", val);
			break;

		case 2:
			printf("%06d ", val);
			break;

		case 3:
			printf("%04x ", val);
			break;
		}
	}
}

static void 
_print_line_long(const char *buf, int num)
{
	int i;
	unsigned int val;

	for (i = 0; i < num; i += 4) {
		
		val = *(unsigned int *)(buf + i);
		switch (_g_ftype) {

		case 1:
			printf("%012o ", val);
			break;

		case 2:
			printf("%012d ", val);
			break;

		case 3:
			printf("%08x ", val);
			break;
		}
	}
}

static void 
_print_line(int offset, const char *buf, int num)
{
	switch (_g_offset) {
	case 0:
		printf("%06x ", offset);
		break;

	case 1:
		printf("%06o ", offset);
		break;

	case 2:
		printf("%06d ", offset);
		break;

	case 3:
		printf("%06x ", offset);
		break;
	}

	switch (_g_fnum) {

	case 1:
		if (_g_ftype == 0)
			_print_line_char(buf, num);
		else
			_print_line_byte(buf, num);
		break;

	case 2:
		_print_line_short(buf, num);
		break;

	case 4:
		_print_line_long(buf, num);
		break;
	}
	printf("\n");

}

/**
 *	The main entry of program.	
 *
 * 	Return 0 if success, other value on error.
 */
int 
main(int argc, char **argv)
{
	int fd;
	int m, n;
	int offset = 0;
	char buf[16];

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	fd = open(_g_file, O_RDONLY);
	if (fd < 0) {
		printf("can't open file %s: %s\n", _g_file, strerror(errno));
		return -1;
	}

	lseek(fd, _g_skip, SEEK_SET);

	n = 0;
	offset += _g_skip;
	while (1) {
		m = read(fd, buf, _g_width);
		if (m <= 0)
			break;

		_print_line(offset, buf, m);
		n += m;
		offset += m;
		if (_g_dump > 0 && n >= _g_dump)
			break;
	}

	_release();

	return 0;
}



