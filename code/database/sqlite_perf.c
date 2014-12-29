/**
 *	@file	sqlite_perf.c	
 *
 *	@brief	the sqlite performace test program
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

#include <sqlite3.h>

#define	SINGLE_MODE	0
#define	BATCH_MODE	1

static int	_g_max_record = 100000;		/* default is max record */
static int 	_g_mode	= SINGLE_MODE;
static sqlite3	*_g_db = NULL;

typedef struct db_record {
	u_int32_t	id;
	char		name[16];
	u_int16_t	age;
	u_int16_t	level;
	char		addr[128];
	u_int16_t	msglen;
	char		msg[0];
} db_record_t;


/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("sqlite_perf <options>\n");
	printf("\t-t\tusing batch mode, insert many records one time, "
	       "default is single mode\n");
	printf("\t-n\tmax record number\n");
	printf("\t-s\tthe record size\n");
	printf("\t-c\tinsert number in one transation\n");
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
	char optstr[] = ":tn:s:c:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {

		case 'b':
			_g_mode = BATCH_MODE;
			break;

		case 'n':
			_g_db_count = atoi(optarg);
			if (_g_db_count < 1) {
				return -1;
			}
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
	/* remove old database */
	if (access("test.db") == 0)
		unlink("test.db");

	/* create a new database */
	_g_db = sqlite3_open(test.db);
	if (!_g_db) {
		printf("open sqlite database file error\n");
		return -1;
	}
	
	/* create a new table */
	_g_db

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



/**
 *	@file	
 *
 *	@brief
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */





