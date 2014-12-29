/**
 *	@file	confdata.h
 *
 *	@brief	The config data storage APIs.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2009-01-13
 */

#ifndef FZ_CONFDATA_H
#define FZ_CONFDATA_H

/**
 *	CLI basic object value.
 */
typedef struct cli_data {
	u_int32_t	nused;	/* how many other object used it */
	u_int32_t	len;	/* data len */
	char		data[0];
} cli_data_t;

/**
 *	CLI unique object value
 */
typedef struct cli_unidata {
	cli_object_t	*obj;
	cli_data_t	*data;
} cli_unidata_t;

/**
 *	CLI table object value
 */
typedef struct cli_tabledata {
	cli_object_t	*obj;
	cli_data_t	*vals;
	u_int32_t	nvals;
} cli_tabledata_t;

/**
 *	CLI config value storage structure.
 */
typedef struct cli_confdata {
	cli_data_t	*unidata;
	u_int32_t	tablesize;
	u_int32_t	nused;
	cli_data_t	*tabledata;	
} cli_confdata_t;

extern cli_confdata_t * 
cli_create_confdata(void);

extern void 
cli_free_confdata(void);

extern int 
cli_add_unidata(cli_confdata_t *conf, cli_unidata_t *data);

extern int 
cli_add_tabledata(cli_confdata_t *conf, cli_tabledata_t *data);

extern int 
cli_modify_unidata(cli_confdata_t *conf, cli_unidata_t *data);

extern int 
cli_modify_tabledata(cli_confdata_t *conf, cli_tabledata_t *data);

extern int
cli_del_tabledata(cli_confdata_t *conf, cli_tabledata_t *data);


#endif /* end of FZ_  */

