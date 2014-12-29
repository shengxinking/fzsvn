/**
 *	@file	mysql_api.c
 *
 *	@brief	mysql APIs implement.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2013-04-29
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "database.h"

//#define	_MYSQL_DEBUG		1

#ifdef	_MYSQL_DEBUG
#define	_MYSQL_DBG(fmt, arg...)		printf("[mysql]: "fmt, ##arg)
#else
#define	_MYSQL_DBG(fmt, arg...)
#endif

#define	_MYSQL_ERR(fmt, arg...)		\
	printf("[mysql][%s:%d]: "fmt, __FILE__, __LINE__, ##arg)

static int 
_db_mysql_callback(db_mysql_t *ctx, db_cbfunc cb, void *arg)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	MYSQL_FIELD *col;
	db_value_t *vals;
	int ncol;
	int ret = 0;
	int i;

	res = mysql_store_result(&ctx->connection);
	if (!res)
		return -1;
	
	ncol = mysql_num_fields(res);
	if (ncol < 0) {
		mysql_free_result(res);
		return -1;
	}

	vals = malloc(sizeof(db_value_t) * ncol);
	if (!vals)
		return -1;

	while ((row = mysql_fetch_row(res))) {
	
		memset(vals, 0, sizeof(db_value_t) * ncol);
		
		for (i = 0; i < ncol; i++) {
			col = mysql_fetch_field_direct(res, i);
			vals[i].name = col->name;
			switch(col->type) {
			case FIELD_TYPE_TINY:
			case FIELD_TYPE_SHORT:
			case FIELD_TYPE_LONG:
			case FIELD_TYPE_INT24:
			case FIELD_TYPE_LONGLONG:
			case FIELD_TYPE_DECIMAL:
				vals[i].type = DB_LONG;
				if (row[i])
					vals[i].val.lval = strtol(row[i], NULL, 10);
				break;
			case FIELD_TYPE_FLOAT:
			case FIELD_TYPE_DOUBLE :
				vals[i].type = DB_DOUBLE;
				if (row[i])
					vals[i].val.dval = strtod(row[i], NULL);
				break;
			case FIELD_TYPE_NULL:
				vals[i].type = DB_NULL;
				break;
			default:
				vals[i].type = DB_STRING;
				vals[i].val.sval = (unsigned char *)row[i];
				break;
			} 
			vals[i].len = col->length;
		}

		if (cb(arg, ncol, vals))
			break;
	}
	free(vals);
	mysql_free_result(res);

	return ret;
}

int  
db_mysql_open_remote(db_mysql_t *ctx, const char *server, u_int16_t port, 
	      const char *user, const char *pass, const char *dbname)
{
	if (!ctx || !user || !pass) {
		_MYSQL_ERR("invalid args\n");
		return -1;
	}

	if (!mysql_init(&ctx->connection)) {
		_MYSQL_ERR("mysql_init failed\n");
		return -1;
	}

	if (!mysql_real_connect(&ctx->connection, server, user, pass, 
				dbname, port, NULL, 0))
	{
		_MYSQL_ERR("connect to %s:%d failed\n", server, port);
		return -1;
	}

	return 0;
}

int 
db_mysql_open_local(db_mysql_t *ctx, const char *path, const char *user,
		    const char *pass, const char *dbname)
{
	if (!ctx || !user || !pass) {
		_MYSQL_ERR("invalid args\n");
		return -1;
	}

	if (!mysql_init(&ctx->connection)) {
		_MYSQL_ERR("mysql_init failed\n");
		return -1;
	}

	if (!mysql_real_connect(&ctx->connection, NULL, user, pass, 
				dbname, 0, path, 0))
	{
		_MYSQL_ERR("connect to %s failed\n", path);
		return -1;
	}

	return 0;
}

int 
db_mysql_exec(db_mysql_t *ctx, const char *sql, db_cbfunc cb, void *arg)
{
	int ret;

	if (!ctx || !sql)
		return -1;

	ret = mysql_query(&ctx->connection, sql);
	_MYSQL_DBG("execute: %s\n", sql);
	if (ret) {
		_MYSQL_ERR("execute SQL failed: %s\n", sql);
		return -1;
	}
	
	if (cb) {
		_db_mysql_callback(ctx, cb, arg);
	}

	return 0;
}

int 
db_mysql_close(db_mysql_t *ctx)
{
	if (!ctx)
		return -1;

	mysql_close(&ctx->connection);

	return 0;
}



