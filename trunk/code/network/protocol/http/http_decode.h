/**
 *	@file	http_decode.h
 *
 *	@brief	HTTP decode function.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_HTTP_DECODE_H
#define FZ_HTTP_DECODE_H


/**
 *	Decode a string @str and save it to @decstr
 *	
 *	Return 0 if success, -1 on error.
 */
extern int 
http_decode_str(const char *str, size_t len, char *decstr, size_t *declen);


/**
 *	Decode a string @str and save it to @decstr, it'll decode again
 *	until no decode found.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
http_decode_str2(const char *str, size_t len, char *decstr, size_t *declen);

/**
 * 	Decode amf buffer @buf and save it to @decbuf.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
http_decode_amf(const void *buf, size_t len, void *decbuf, size_t *declen);


/**
 * 	Decode URL path @buf and save it to @decbuf.
 *
 * 	Return 0 if successs, -1 on error.
 */
extern int 
http_decode_urlpath(const void *buf, size_t len, void *decbuf, size_t *declen);


#endif /* end of FZ_DECODE_H  */


