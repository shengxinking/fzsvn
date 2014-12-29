/**
 *	@file	database_test.c
 *
 *	@brief	the Database test program.
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
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "database.h"

#define	SQL_LOAD_FMT	\
	"load data infile '%s' into table %s fields terminated by ','"
#define	SQL_INSERT_FMT		\
	"insert into %s(fileid,offset,date,time) values(%d,%d,%d,%d)"
#define SQL_SELECT_FMT		\
	"select %s from %s where %s"
#define SQL_GROUP_FMT		\
	"select %s from %s group by %s"

enum {
	DB_MYSQL,
	DB_SQLITE,
};

static int		_g_dbtype;	/* database type: Mysql/Sqlite */
static char		_g_dbserver[32];/* mysql server address|sqlite path */
static char		_g_dbuser[32];	/* username */
static char		_g_dbpass[32];	/* password */
static char		_g_dbname[32];	/* database name */
static char		*_g_sqlbuf;	/* buffer for store sql file */
static char		_g_tbname[128];	/* the table name */
static char		_g_datafile[128];/* data file for IMPORT DATA */
static char		_g_sqlfile[128];/* SQL file for insert */
static char		_g_sqlstr[512];	/* SQL select statement */

static char _g_optstr[] = ":t:a:u:p:d:n:L:I:S:h";

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("database_test <options>\n");
	printf("\t-t\tdatabase type: mysql|sqlite, default is mysql\n");
	printf("\t-a\tmysql server address | sqlite database filename\n");
	printf("\t-u\tusername\n");
	printf("\t-p\tpassword\n");
	printf("\t-d\tdatabase name\n");
	printf("\t-n\tthe table name\n");
	printf("\t-L\tthe data file for LOAD DATA test\n");
	printf("\t-I\tthe sql file for INSERT test\n");
	printf("\t-S\tthe sql statement for SELECT test\n");
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

		case 't':
			if (strcmp("mysql", optarg) == 0)
				_g_dbtype = DB_MYSQL;
			else if (strcmp("sqlite", optarg) == 0)
				_g_dbtype = DB_SQLITE;
			else {
				printf("unknow database %s\n", optarg);
				return -1;
			}

			break;

		case 'a':
			strncpy(_g_dbserver, optarg, sizeof(_g_dbserver) - 1);
			break;

		case 'u':
			strncpy(_g_dbuser, optarg, sizeof(_g_dbuser)-1);
			break;

		case 'p':
			strncpy(_g_dbpass, optarg, sizeof(_g_dbpass)-1);
			break;
			
		case 'd':
			strncpy(_g_dbname, optarg, sizeof(_g_dbname)-1);
			break;

		case 'n':
			strncpy(_g_tbname, optarg, sizeof(_g_tbname)-1);
			break;

		case 'L':
			strncpy(_g_datafile, optarg, sizeof(_g_datafile)-1);
			break;

		case 'I':
			strncpy(_g_sqlfile, optarg, sizeof(_g_sqlfile)-1);
			break;
		
		case 'S':
			strncpy(_g_sqlstr, optarg, sizeof(_g_sqlstr) - 1);
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

	if (strlen(_g_datafile) > 0) {
		printf("111111\n");
		if (strlen(_g_tbname) == 0) {
			printf("load data need provide table name\n");
			return -1;
		}
	}
	else if (strlen(_g_sqlfile) < 1 && strlen(_g_sqlstr) < 1) {
		printf("need use option -I or -S\n");
		return -1;
	}

	if (strlen(_g_dbserver) < 1) {
		printf("need use option -a\n");
		return -1;
	}
	if (_g_dbtype == DB_MYSQL) {
		if (strlen(_g_dbuser) < 1) {
			printf("no database user\n");
			return -1;
		}
		if (strlen(_g_dbname) < 1) {
			printf("no database name\n");
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
	long size;
	int fd;
	struct stat st;

	/* load SQL file into buffer */
	if (strlen(_g_sqlfile) > 0) {
		if (stat(_g_sqlfile, &st)) {
			printf("open SQL file %s failed\n", _g_sqlfile);
			return -1;
		}
		size = st.st_size;

		_g_sqlbuf = malloc(size + 1);
		if (!_g_sqlbuf) {
			printf("alloc memory %ld failed\n", size + 1);
			return -1;
		}

		fd = open(_g_sqlfile, O_RDONLY);
		if (fd < 0) {
			printf("open SQL file %s failed\n", _g_sqlfile);
			return -1;
		}

		if (read(fd, _g_sqlbuf, size) != size) {
			printf("read SQL file %s failed\n", _g_sqlfile);
			close(fd);
			return -1;
		}

		_g_sqlbuf[size] = 0;

		close(fd);
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
	/* free SQL buffer */
	if (_g_sqlbuf)
		free(_g_sqlbuf);
	_g_sqlbuf = NULL;
}

static int 
_timeval_sub(struct timeval *tv1, struct timeval *tv2)
{
	int usec;

	if (!tv1 || !tv2)
		return 0;

	usec = (tv2->tv_sec - tv1->tv_sec) * 1000000 ;
	usec += (tv2->tv_usec - tv1->tv_usec);

	return usec;
}

static int 
_callback(void *arg, int ncol, const db_value_t *vals)
{
	int i;

	if (!vals)
		return -1;

	for (i = 0; i < ncol; i++) {
		switch (vals[i].type) {
		case DB_LONG:
			printf("%s(%d.%lu)=%ld,", vals[i].name, 
			       vals[i].type, vals[i].len, vals[i].val.lval);
			break;
		case DB_DOUBLE:
			printf("%s(%d.%lu)=%f,", vals[i].name, 
			       vals[i].type, vals[i].len, vals[i].val.dval);
			break;
		case DB_STRING:
			printf("%s(%d.%lu)=%s,", vals[i].name, 
			       vals[i].type, vals[i].len, vals[i].val.sval);
			break;
		case DB_NULL:
			printf("%s(%d.%lu)=NULL,", vals[i].name,
			       vals[i].type, vals[i].len);
			break;
		default:
			printf("error type %d\n", vals[i].type);
			break;
		}
	}
	printf("\n");
	return 0;
}

static int 
_mysql_test(void)
{
	db_mysql_t mysql;
	char sql[1024];
	int ret;
	int usec;
	long cnt = 0;
	char *ptr, *begin;
	struct timeval start, end;

	if (db_mysql_open_remote(&mysql, _g_dbserver, 0, 
				 _g_dbuser, _g_dbpass, _g_dbname))
		return -1;

	/* test load data performance */
	if (strlen(_g_datafile) > 0) {
		printf("load data performance\n");
		snprintf(sql, sizeof(sql), SQL_LOAD_FMT, 
			 _g_datafile, _g_tbname);
	
		gettimeofday(&start, NULL);
		ret = db_mysql_exec(&mysql, sql, NULL, NULL);
		gettimeofday(&end, NULL);
		if (ret) {
			printf("execute sql command failed: %s\n", sql);
			db_mysql_close(&mysql);
			return -1;
		}
		usec = _timeval_sub(&start, &end);
		printf("load data spend %d.%d second\n", usec/1000000, usec%1000000);
	}
	/* test INSERT data */
	else if (_g_sqlbuf) {
		/* test insert performance */
		printf("insert performance\n");
		gettimeofday(&start, NULL);
		begin = _g_sqlbuf;
		while ((ptr = strchr(begin, '\n'))) {
			*ptr = 0;
			if (db_mysql_exec(&mysql, begin, NULL, NULL)) {
				printf("execute sql failed: %s\n", begin);
				break;
			}
			begin = ptr + 1;
			*ptr = '\n';
			cnt++;
		}
		gettimeofday(&end, NULL);
		usec = _timeval_sub(&start, &end);
		printf("insert (%ld) spend %d.%d second\n", 
		       cnt, usec/1000000, usec%1000000);
	}
	else if (strlen(_g_sqlstr) > 0) {
		gettimeofday(&start, NULL);
		ret = db_mysql_exec(&mysql, _g_sqlstr, _callback, NULL);
		gettimeofday(&end, NULL);
		if (ret) {
			printf("execute sql failed: %s\n", _g_sqlstr);
			db_mysql_close(&mysql);
			return -1;
		}
		usec = _timeval_sub(&start, &end);
		printf("select spend %d.%d second\n", usec/1000000, usec%1000000);
	}

	db_mysql_close(&mysql);

	return 0;
}

static int 
_sqlite_test(void)
{
	db_sqlite_t sqlite;
	struct timeval start, end;
	int usec;
	long cnt = 0;
	int ret;
	
	if (db_sqlite_open(&sqlite, _g_dbserver)) {
		printf("open database failed\n");
		return -1;
	}
	
	if (_g_sqlbuf) {
		/* test insert performance */
		printf("insert performance\n");
		ret = db_sqlite_exec(&sqlite, "PRAGMA synchronous=1", NULL, NULL);
		if (ret) {
			printf("execute transaction failed\n");
			db_sqlite_close(&sqlite);
			return -1;
		}
		ret = db_sqlite_exec(&sqlite, "PRAGMA cache_size=1;", NULL, NULL);
		if (ret) {
			printf("execute transaction failed\n");
			db_sqlite_close(&sqlite);
			return -1;
		}
		ret = db_sqlite_exec(&sqlite, "PRAGMA page_size=1024", NULL, NULL);
		if (ret) {
			printf("execute transaction failed\n");
			db_sqlite_close(&sqlite);
			return -1;
		}
		ret = db_sqlite_exec(&sqlite, "PRAGMA temp_store=2", NULL, NULL);
		if (ret) {
			printf("execute transaction failed\n");
			db_sqlite_close(&sqlite);
			return -1;
		}
		ret = db_sqlite_exec(&sqlite, "begin transaction", NULL, NULL);
		if (ret) {
			printf("execute transaction failed\n");
			db_sqlite_close(&sqlite);
			return -1;
		}
		gettimeofday(&start, NULL);
		ret = db_sqlite_exec(&sqlite, _g_sqlbuf, NULL, NULL);
		gettimeofday(&end, NULL);
		if (ret) {
			printf("execute insert sql failed\n");
			db_sqlite_close(&sqlite);
			return -1;
		}
		ret = db_sqlite_exec(&sqlite, "commit", NULL, NULL);
		if (ret) {
			printf("execute transaction failed\n");
			db_sqlite_close(&sqlite);
			return -1;
		}
		usec = _timeval_sub(&start, &end);
		printf("insert (%ld) spend %d.%d second\n", 
		       cnt, usec/1000000, usec%1000000);
	}
	else if (strlen(_g_sqlstr) > 0) {
		gettimeofday(&start, NULL);
		if (db_sqlite_exec(&sqlite, _g_sqlstr, _callback, NULL)) {
			printf("execute sql failed: %s\n", _g_sqlstr);
			db_sqlite_close(&sqlite);
			return -1;
		}
		gettimeofday(&end, NULL);
		usec = _timeval_sub(&start, &end);
		printf("select spend %d.%d second\n", usec/1000000, usec%1000000);
	}
	
	db_sqlite_close(&sqlite);

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

	switch (_g_dbtype) {
	case DB_MYSQL:
		_mysql_test();
		break;

	case DB_SQLITE:
		_sqlite_test();
		break;

	default:
		_usage();
		return -1;
	}

	_release();

	return 0;
}



