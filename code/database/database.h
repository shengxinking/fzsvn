/**
 *	@file	database.h
 *
 *	@brief	The database APIs defined.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2013-05-13
 */

#ifndef FZ_DATABASE_H
#define FZ_DATABASE_H

enum {
	DB_LONG,
	DB_DOUBLE,
	DB_STRING,
	DB_NULL,
};

typedef struct db_value {
	const char	*name;		/* the column name */
	int		type;		/* the value type */
	size_t		len;		/* the value length value(convert to string) */
	union {
		long		lval;
		double		dval;
		const u_int8_t	*sval;
	} val;
} db_value_t;

typedef	int (*db_cbfunc)(void *arg, int ncol, const db_value_t *val);


/**********************************************************
 *	Mysql APIs
 *********************************************************/
#include <mysql/mysql.h>

typedef struct db_mysql {
	MYSQL		connection;	/* the connection to database */
} db_mysql_t;

/**
 *	Connect to remote MYSQL database. the MYSQL database 
 *	server address is @server, port is @port, username
 *	is @user, password is @pass, the database name is
 *	@dbname.
 *
 *	Return 0 if success, -1 on error.
 */
extern int  
db_mysql_open_remote(db_mysql_t *ctx, const char *server, u_int16_t port, 
		     const char *user, const char *pass, const char *dbname);

/**
 *	Connect to local MYSQL database. the MYSQL database 
 *	server path is @path, port is @port, username
 *	is @user, password is @pass, the database name is
 *	@dbname.
 *
 *	Return 0 if success, -1 on error.
 */
extern int  
db_mysql_open_local(db_mysql_t *ctx, const char *path, const char *user, 
		    const char *pass, const char *dbname);

/**
 *	Check the mysql_ctx_t @ctx is connected or not.
 *
 *	Return 1 if is connected, 0 if not.
 */
extern int 
db_mysql_is_opened(db_mysql_t *ctx);

/**
 *	Close mysql database connection and all changes are commit
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
db_mysql_close(db_mysql_t *ctx);

/**
 *	Execute a SQL statements @sql, if @cb is not NULL, it'll 
 *	call function @cb(if not NULL) in each single SQL statement, 
 *	if @cb return non-zero, it'll abort and not execute next SQL
 *	statement. the fourth argument @arg is the first argument
 *	for @cb.
 *
 *	Return 0 if all SQL statements run success, -1 on error. 
 */
extern int 
db_mysql_exec(db_mysql_t *ctx, const char *sql, db_cbfunc cb, void *arg);

/**********************************************************
 *	Sqlite3 APIs
 *********************************************************/
#include <sqlite3.h>

typedef struct db_sqlite {
	sqlite3		*db;
} db_sqlite_t;

/**
 *	Open the sqlite3 database file @dbfile, if @dbfile
 *	not exist, create a empty database.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
db_sqlite_open(db_sqlite_t *ctx, const char *dbfile);

/**
 *	Close the sqlite3 database @ctx.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
db_sqlite_close(db_sqlite_t *ctx);

/**
 *	Execute a SQL statements @sql, if @cb is not NULL, it'll 
 *	call function @cb(if not NULL) in each single SQL statement, 
 *	if @cb return non-zero, it'll abort and not execute next SQL
 *	statement. the fourth argument @arg is the first argument
 *	for @cb.
 *
 *	Return 0 if all SQL statements run success, -1 on error. 
 */
extern int 
db_sqlite_exec(db_sqlite_t *ctx, const char *sql, db_cbfunc db, void *arg);


#endif /* end of FZ_DATABASE_H  */

