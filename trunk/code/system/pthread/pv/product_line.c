/**
 *	@file	product_line.c
 *
 *	@brief	A simple product line program.
 *
 *	@author	Forrest.zhang
 *	
 *	@date	2010-06-03
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>

#define MAX_PARTS	6

typedef enum worker_type {
	WORKER_START = 0,
	WORKER_RECV,
	WORKER_PARSE,
	WORKER_SEND,
} work_type_t;

typedef enum parttype {
	PART_SHELL = 0,
	PART_WHEEL,
	PART_ENGINE,
} parttype_t;

typedef struct motopart {
	u_int32_t	id;
	int		type;	
} motopart_t;

typedef struct moto {
	u_int32_t	id;
	int		nparts;
	int		shell;
	int		wheel;
	int		engine;
	int		nbuilds;
	pthread_mutex_t lock;
} moto_t;

typedef struct prodline {
	void		*cache;
	void		*hash;
	moto_t		*flist;
	u_int32_t	freeid;
	u_int32_t	capacity;
	pthread_rwlock_t lock;
} prodline_t;


static prodline_t *
prodline_alloc(int capacity)
{

}


static void 
prodline_free(prodline_t *pl)
{

}


static moto_t *
prodline_new(prodline_t *pl)
{
	moto_t *m = NULL;

	return m;
}

static void 
prodline_del(prodline_t *pl, moto_t *m)
{

}


static moto_t *
prodline_find(prodline_t *pl, u_int32_t id)
{
	moto_t *m = NULL;

	return m;
}

static int
prodline_put(prodline_t *pl, moto_t *m)
{


}


/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
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
	char optstr[] = ":h";
	
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



