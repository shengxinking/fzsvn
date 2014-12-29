/**
 *	@file	sqlite_api.c
 *
 *	@brief	The sqlite APIs.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2013-05-01
 */


#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sqlite3.h>

#include "database.h"

//#define	_SQLITE_DEBUG		1

#ifdef	_SQLITE_DEBUG
#define	_SQLITE_DBG(fmt, arg...)		printf("[mysql]: "fmt, ##arg)
#else
#define	_SQLITE_DBG(fmt, arg...)
#endif

#define	_SQLITE_ERR(fmt, arg...)		\
	printf("[mysql][%s:%d]: "fmt, __FILE__, __LINE__, ##arg)

static int 
_db_sqlite_callback(sqlite3_stmt *stmt, db_cbfunc cb, void *arg)
{
	db_value_t *vals;
	int dtype;
	int ncol;
	int i;
	int ret;
	
	ncol = sqlite3_column_count(stmt);
	if (ncol < 0) {
		return -1;
	}
	vals = malloc(ncol * sizeof(db_value_t));
	if (!vals) {
		_SQLITE_ERR("malloc memory failed\n");
		return -1;
	}
	memset(vals, 0, ncol * sizeof(db_value_t));
				
	for (i = 0; i < ncol; i++) {
		vals[i].name = sqlite3_column_name(stmt, i);
		vals[i].len = sqlite3_column_bytes(stmt, i);
		dtype = sqlite3_column_type(stmt, i);
		switch (dtype) {
		case SQLITE_INTEGER:
			vals[i].type = DB_LONG;
			vals[i].val.lval = sqlite3_column_int(stmt, i);
			break;
		case SQLITE_FLOAT:
			vals[i].type = DB_DOUBLE;
			vals[i].val.dval = sqlite3_column_double(stmt, i);
			break;
		case SQLITE_TEXT:
		case SQLITE_BLOB:
			vals[i].type = DB_STRING;
			vals[i].val.sval = sqlite3_column_text(stmt, i);
			break;
		case SQLITE_NULL:
			vals[i].type = DB_NULL;
			break;
		default:
			_SQLITE_ERR("unknowed data type: %d", dtype);
			return -1;
		}
	}
				
	ret = cb(arg, ncol, vals);
	if (vals) 
		free(vals);

	return ret;
}

int 
db_sqlite_open(db_sqlite_t *ctx, const char *dbfile)
{
	int ret;
	int flags = SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX;

	if (!ctx || !dbfile)
		return -1;
	
	memset(ctx, 0, sizeof(db_sqlite_t));

	ret = sqlite3_open_v2(dbfile, &ctx->db, flags, NULL);
	if (ret != SQLITE_OK || ctx->db == NULL) {
		db_sqlite_close(ctx);
		return -1;
	}

	return 0;
}

int 
db_sqlite_close(db_sqlite_t *ctx)
{
	int ret;

	if (!ctx)
		return -1;

	if (ctx->db) {
		ret = sqlite3_close(ctx->db);
		ctx->db = NULL;
		if (ret != SQLITE_OK)
			ret = -1;
	}

	return (ret == SQLITE_OK ? 0 : -1);
}


int 
db_sqlite_exec(db_sqlite_t *ctx, const char *sql, db_cbfunc cb, void *arg)
{
	sqlite3_stmt *stmt = NULL;
	const char *tail = NULL;
	const char *begin = sql;
	int ret = 0;

	if (!ctx || !sql || !ctx->db)
		return -1;

	while (begin) {
		/* compile SQL statement */
		ret = sqlite3_prepare_v2(ctx->db, begin, -1, &stmt, &tail);		
		if (ret != SQLITE_OK || !stmt) {
//			_SQLITE_ERR("sqlite3_prepare_v2 failed: %s\n", begin);
			ret = SQLITE_OK;
			break;
		}
		
		/* run SQL statement until error or done */
		do {
			ret = sqlite3_step(stmt);
			/* need callback funtion process data */
			if (ret == SQLITE_ROW && cb) {
				if (_db_sqlite_callback(stmt, cb, arg))
					break;
			}
			else if (ret == SQLITE_BUSY){
				sqlite3_reset(stmt);
			}
		} while (ret == SQLITE_BUSY || ret == SQLITE_ROW);
		
		ret = sqlite3_finalize(stmt);
		stmt = NULL;

		if (ret != SQLITE_OK) {
			_SQLITE_ERR("sqlite_finalize failed: %d\n", ret);
			break;
		}
		
		/* check tail is end */
		begin = tail;
		if (strlen(begin) < 1)
			break;
	}

	if (stmt) {
		sqlite3_finalize(stmt);
		stmt = NULL;
	}
	
	return (ret == SQLITE_OK ? 0 : -1);
}

