/**
 * file         http_protocol.h
 *
 * brief        a simple HTTP protocol implement
 *
 * author       Forrest.zhang
 */

#ifndef FZ_HTTP_PROTOCOL_H
#define FZ_HTTP_PROTOCOL_H

#include <sys/types.h>

/* control debug print */
#define	_HTTP_DEBUG	1


#define HTTP_CACHE_MAX                  32	/* http state-machine cache */
#define	HTTP_BUF_MAX			8192	/* buffer for parameters */
#define HTTP_METHOD_MAX                 8	/* the max method */
#define	HTTP_COOKIE_MAX			32	/* the max cookie number */
#define	HTTP_ARGUMENT_MAX		32	/* the max argument number */
#define	HTTP_HEADLINE_MAX		32	/* the max headline number */


/**
 *	Some expecial char CR, LF, SPACE, Horizon Table, QUOTE
 */
#define HTTP_CHR_CR                     13
#define HTTP_CHR_LF                     10
#define HTTP_CHR_SP                     32
#define HTTP_CHR_HT                     9
#define HTTP_CHR_QUOTE			34
#define HTTP_CHR_PERCENT		37
#define HTTP_CHR_EQUAL			61

/**
 *	HTTP version, 1.0 and 1.1
 */
#define	HTTP_VER_09			9
#define HTTP_VER_10                     10
#define HTTP_VER_11                     11
#define HTTP_VER_UNKOWNED               255

/**
 *	HTTP request method, GET, PUT, POST, HEAD is the common 
 */
#define HTTP_MED_OPTIONS                1
#define HTTP_MED_GET                    2
#define HTTP_MED_HEAD                   3
#define HTTP_MED_POST                   4
#define HTTP_MED_PUT                    5
#define HTTP_MED_DELETE                 6
#define HTTP_MED_TRACE                  7
#define HTTP_MED_CONNECT                8
#define HTTP_MED_EXTENSION              255


/**
 *	HTTP main state for parse 
 */
#define HTTP_STE_BEGIN                  0	/* HTTP message begin */
#define HTTP_STE_METHOD                 1	/* request method */
#define HTTP_STE_VERSION                2	/* version string */
#define HTTP_STE_URL                    3	/* request URL */
#define HTTP_STE_CRLF                   4	/* CR-LF as head line end */
#define HTTP_STE_HFIELD                 5	/* header line field */
#define HTTP_STE_HVALUE                 6	/* header line value */
#define HTTP_STE_BODY                   7	/* body */
#define HTTP_STE_FIN                    8	/* HTTP message finished */
#define HTTP_STE_CODE                   9       /* HTTP response code */
#define HTTP_STE_REASON                 10      /* HTTP response reason */


/**
 *	HTTP general token state, it's used parse HTTP header
 */
#define HTTP_TST_BEGIN                  0
#define HTTP_TST_IN                     1
#define HTTP_TST_LWS			4
#define HTTP_TST_FIN                    5

/**
 *	HTTP CRLF state for parse, it's only in header line end 
 */
#define HTTP_TST_CR                     6
#define HTTP_TST_LF                     7

/**
 *	HTTP header fields: general 
 */
#define HTTP_HST_BEGIN                  0
#define HTTP_HST_CACHE_CONTROL          11
#define HTTP_HST_CONNECTION             12
#define HTTP_HST_DATE                   13
#define HTTP_HST_PRAGMA                 14
#define HTTP_HST_TRAILER                15
#define HTTP_HST_TRANSFER_ENCODING      16
#define HTTP_HST_UPGRADE                17
#define HTTP_HST_VIA                    18
#define HTTP_HST_WARNING                19

/** 
 *	HTTP header fields: request 
 */
#define HTTP_HST_ACCEPT                 31
#define HTTP_HST_ACCEPT_CHARSET         32
#define HTTP_HST_ACCEPT_ENCODING        33
#define HTTP_HST_ACCEPT_LANGUAGE        34
#define HTTP_HST_AUTHORIZATION          35
#define HTTP_HST_EXPECT                 36
#define HTTP_HST_FROM                   37
#define HTTP_HST_HOST                   38
#define HTTP_HST_IF_MATCH               39
#define HTTP_HST_IF_MODIFIED_SINCE      40
#define HTTP_HST_IF_NONE_MATCH          41
#define HTTP_HST_IF_RANGE               42
#define HTTP_HST_IF_UNMODIFIED_SINCE    43
#define HTTP_HST_MAX_FORWARDS           44
#define HTTP_HST_PROXY_AUTHORIZATION    45
#define HTTP_HST_RANGE                  46
#define HTTP_HST_TE                     47
#define HTTP_HST_USER_AGENT             48
#define HTTP_HST_COOKIE			49
#define HTTP_HST_REFERENCE		50
#define HTTP_HST_XFF			51


/**
 *	HTTP header fields: response 
 */
#define HTTP_HST_ACCEPT_RANGES          61
#define HTTP_HST_AGE                    62
#define HTTP_HST_ETAG                   63
#define HTTP_HST_LOCATION               64
#define HTTP_HST_PROXY_AUTHENTICATE     65
#define HTTP_HST_RETRY_AFTER            66
#define HTTP_HST_SERVER                 67
#define HTTP_HST_VARY                   68
#define HTTP_HST_WWW_AUTHENTICATE       69
#define HTTP_HST_SET_COOKIE		70
#define HTTP_HST_KEEP_ALIVE		71


/**
 *	HTTP header fields: entity 
 */
#define HTTP_HST_ALLOW                  81
#define HTTP_HST_CONTENT_ENCODING       82
#define HTTP_HST_CONTENT_LANGUAGE       83
#define HTTP_HST_CONTENT_LENGTH         84
#define HTTP_HST_CONTENT_LOCATION       85
#define HTTP_HST_CONTENT_MD5            86
#define HTTP_HST_CONTENT_RANGE          87
#define HTTP_HST_CONTENT_TYPE           88
#define HTTP_HST_EXPIRES                89
#define HTTP_HST_LAST_MODIFIED          90

#define HTTP_HST_EXTENSION              255

/* HTTP body form state */
#define	HTTP_BST_FORM_BEGIN		1
#define	HTTP_BST_FORM_HNAME		2
#define	HTTP_BST_FORM_HVALUE		3
#define	HTTP_BST_FORM_BODY		4
#define	HTTP_BST_FORM_END		5

/* http Content-Type */
#define HTTP_CTE_TEXT_HTML              0
#define HTTP_CTE_TEXT_XML               1
#define HTTP_CTE_TEXT_PLAIN		2
#define HTTP_CTE_UNKOWNED               255

#define HTTP_REQUEST			0
#define HTTP_RESPONSE			1

/* http modify type */
#define	HTTP_ADD			1	/* add to first position */
#define	HTTP_DEL			2	/* delete */
#define	HTTP_APPEND			3	/* append to tail position */
#define	HTTP_EDIT			4	/* edit */


/* http error */
#define HTTP_CACHE_OVERSIZE		-1	/* cache oversize */
#define	HTTP_HEADLINE_OVERSIZE		-2	/* headline number oversize */
#define	HTTP_ARGGUMENT_OVERSIZE		-3	/* argument number oversize */
#define	HTTP_COOKIE_OVERSIZE		-4	/* cookie number oversize */
#define	HTTP_METHOD_ERROR		-5	/* method error */
#define	HTTP_URL_ERROR			-6	/* url error */
#define	HTTP_VERSION_ERROR		-7	/* version error */
#define	HTTP_HNAME_ERROR		-8	/* header name error */
#define	HTTP_HVALUE_ERROR		-9	/* headerr value error */
#define	HTTP_CLEN_ERROR			-10	/* content length error */


/* http parameter type */
enum {
	HTTP_STR_URL = 1,
	HTTP_STR_DECURL,
	HTTP_STR_ABSURL,
	HTTP_STR_DECABSURL,
	HTTP_STR_DECPATHURL,
	HTTP_STR_HOST,
	HTTP_STR_DECHOST,
	HTTP_STR_LOCATION,
	HTTP_STR_USER_AGENT,
	HTTP_STR_REFERENCE,
	HTTP_STR_AUTH,
	HTTP_STR_LOCATION_HOST,
	HTTP_STR_LOCATION_URL,
	HTTP_STR_USER,
	HTTP_STR_PASS,
	HTTP_STR_WAFSID,
};


enum {
	HTTP_INT_REQVER = 1,	/* http request version, HTTP_VER_XX */
	HTTP_INT_RESVER,	/* http response version, HTTP_VER_XX */
	HTTP_INT_METHOD,	/* http method */
	HTTP_INT_RETCODE,	/* ret code */
	HTTP_INT_HLEN,		/* header length */
	HTTP_INT_BLEN,		/* body length */
	HTTP_INT_CLEN,		/* content length */
	HTTP_INT_HL_MAX,	/* max header line len */
	HTTP_INT_WHOLE_ARGLEN,	/* whole args length */
	HTTP_INT_URL_ARGLEN,	/* URL args length */
	HTTP_INT_RANGE_CNT,	/* Range number */
	HTTP_INT_DIRECTION,	/* HTTP_REQUEST or HTTP_RESPONSE */
	HTTP_INT_STATE,		/* http state machine state */
};

enum {
	HTTP_IS_MSRPC = 1,	/* is MSRPC message */
	HTTP_IS_AMF,		/* is AMF message */
	HTTP_IS_WAFREDIECT,	/* is WAF rediect */
	HTTP_IS_WAFSID,		/* have fortiwaf sid */
	HTTP_IS_GZIP,		/* content is gziped */
	HTTP_IS_ONLYGZIP,	/* only gziped */
	HTTP_IS_XML,		/* xml content */
	HTTP_IS_WAF,		/* waf content */
};


/**
 *	HTTP buffer, stored all http parameters: \
 *	cookie, argument, host, reference, url, decoded url, host.
 *	we have a set of API to get/store this parameters.
 */
typedef struct http_buffer {
	u_int16_t	capacity;	/* buffer capacity size */
	u_int16_t	start;		/* free space start position */
	u_int16_t	end;		/* free space end position */
	char		*buffer;	/* the space */
} http_buffer_t;


/**
 *	HTTP string stored in http_buffer. it only store the start pos
 *	and string length.
 */
typedef struct http_string {
	u_int16_t	is_encoded:1;	/* is HTTP encoded ? 1: 0 */
	u_int16_t	have_arg:1;	/* have args in URL */
	u_int16_t	not_fin:1;	/* this token is finished or not */
	u_int16_t	ori_start;	/* the start position in origin message */
	u_int16_t	ori_len;	/* the origin length */
	u_int16_t	start;		/* the start position in cache */
	u_int16_t	len;		/* the string length in cache */
} http_string_t ;


/**
 *	HTTP argument structure
 */
typedef struct http_arg_s {
	http_string_t	name;		/* the argument name */
	http_string_t	value;		/* the argument value */
} http_arg_s_t;


/**
 * 	HTTP cookie struct
 */
typedef struct http_cookie_s {
	http_string_t	name;		/* cookie name */
	http_string_t	value;		/* cookie value */
	http_string_t	domain;		/* cookie domain */
	http_string_t	path;		/* cookie path */
	http_string_t	expire;		/* cookie expire */
} http_cookie_s_t;


/**
 *	All http parameters.
 */
typedef struct http_parameter {
	u_int32_t	is_svrclosed:1;	/* is sever-closed */
	u_int32_t	is_keepalive:1;	/* is keep-alive */
	u_int32_t	is_waf:1;	/* is waf message */
	u_int32_t	is_xml:1;	/* is xml message */

	u_int8_t	atype;		/* Accept-Type */
	u_int8_t	req_ctype;	/* Request Content-Type */
	u_int8_t	res_ctype;	/* Response Content-Type */
	u_int8_t	req_version;	/* Request HTTP version */
	u_int8_t	res_version;	/* Response HTTP version */
	u_int8_t	method;		/* request method */
	u_int16_t	range;		/* Request Range number */
	u_int16_t	retcode;	/* Response code */
	u_int16_t	ncookie;	/* number of cookie */
	u_int16_t	nargument;	/* number of argument */
	u_int16_t	nheadline;	/* number of headline */
	u_int16_t	arglen;		/* all argument length */
	u_int16_t	maxhlen;	/* max header line length */
	u_int16_t	urlarglen;	/* URL argument length */
	u_int32_t	clen;		/* Content-Length */	
	u_int32_t	blen;		/* Body length */
	u_int32_t	hlen;		/* Header length */

	http_string_t	oriurl;		/* origin URL */
	http_string_t	decurl;		/* decoded URL */
	http_string_t	absurl;		/* the URL without args */
	http_string_t	decabsurl;	/* decoded URL without args */
	http_string_t	decpathurl;	/* URL decode path like ../../ */
	http_string_t	host;		/* Host */
	http_string_t	dechost;	/* decoded Host */
	http_string_t	agent;		/* User-Agent value */
	http_string_t	xff;		/* X-Forwarded-For value */
	http_string_t	xpad;		/* X-Pad value */
	http_string_t	xrealip;	/* X-Real-IP value */
	http_string_t	xpower;		/* X-Power value */
	http_string_t	reference;	/* Reference value */
	http_string_t	location;	/* Location value */
	http_string_t	location_url;	/* Location URL value */
	http_string_t	location_host;	/* Location Host value */
	http_string_t	auth;		/* Auth value */
	http_string_t	auth_user;	/* Auth-Username value */
	http_string_t	auth_pass;	/* Auth-Password value */
	http_string_t	server;		/* Server value */
	http_string_t	aspnet;		/* Asp-Net value */
	http_string_t	ret_code;	/* Ret-Code string */
	http_string_t	postboundary;	/* Post boundary */
	http_string_t	js_arg;		/* Javascript argument */
	http_string_t	originip;	/* the origin IP */
	http_arg_s_t	arguments[HTTP_ARGUMENT_MAX];	/* arguments */
	http_string_t	tmp;		/* temp string for cookie, argument */

	/* the following is need remove when response is coming */
	http_cookie_s_t	cookies[HTTP_COOKIE_MAX];	/* cookies */
	http_string_t	headlines[HTTP_HEADLINE_MAX];	/* head lines */
	http_string_t	ctype;		/* The content-Type string */
} http_parameter_t;


/**
 *	HTTP state-machine structure.
 */
typedef struct http_state {
	u_int8_t	mstate;		/* main state */
	u_int8_t	pstate;		/* previous main state */
	u_int8_t	hstate;		/* header field state */
	u_int8_t	bstate;		/* the body state, now just form use it */
	u_int8_t	tstate;		/* token state */
	u_int16_t	tokpos;		/* the position in token */
	u_int8_t	inquote:1;	/* in quoted */
	u_int8_t	inslash:1;	/* in slash '\' */
	u_int8_t	encode:1;	/* encoded, need decode */
	u_int8_t	unused:5;	/* the unused flags */

	/* cache some token char for parse */
	u_int8_t	csize;
	char		cache[HTTP_CACHE_MAX];
} http_state_t;


typedef struct http_barg {
	http_arg_s_t	args[HTTP_MAX_ARG];/* body arguments */
	u_int16_t	nargs;		/* number of args */
	http_buf_t	buf;
} http_barg_t;


/**
 * 	HTTP upload file struct.
 */
typedef struct http_upfile {
	struct http_upfile *next;	/* list to next upfile */
	char		*name;		/* the uploaded file name, stored in @buf */
	size_t		filelen;	/* the file length */
	char		*sig;		/* the uploaded file first 512 bytes as file signature */
	size_t		len;		/* the buffer length */
	char		buf[0];		/* the buffer stored name, value */
} http_upfile_t;


/**
 *	HTTP all parsed information structure.
 */
typedef struct http_info {
	u_int8_t	msg_type:1;	/* message type: 0 Req, 1 Res */

	/* for state machine, one for main state, one for header state */
	http_state_t	state;

	u_int32_t	pos;		/* the pos in parser string */
	u_int32_t	line;		/* the line number */
	u_int32_t	col;		/* the column number */	

	/* http body infomation */
	u_int32_t	bstart;
	u_int32_t	bremain;

	http_parameter_t para;		/* all http parameter */
	http_upfile_t	*upfile;	/* the uploaded file list */
	http_buffer_t	buffer;		/* buffer for store parameter */
} http_info_t;


typedef struct http_arg {
	char		*name;
	char		*value;
} http_arg_t;

typedef struct http_cookie {
	char		*name;
	char		*value;
	char		*domain;
	char		*path;
	char		*expire;
} http_cookie_t;


/** 
 *	Parse HTTP request, the data is in @buf, begin with @start, the
 *	@buf length is @siz, the parse result is stored in @req.
 *
 *	Return 0 if success, -1 on error and the error number is stored
 *	in @req->errno.
 */
extern int 
http_parse_request(http_info_t *req, const char *buf, 
		   size_t siz, size_t start);

/**
 *	Parse HTTP response, the data is in @buf, begin with @start. the
 *	@buf length is @siz, the parse result is stored in @res.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
http_parse_response(http_info_t *res, const char *buf, size_t siz, size_t start);


/**
 *	Return HTTP reponse status code description.
 *
 *	Return a const string of status code desription.
 */
const char *
http_code2reason(int status_code);


/**
 *	Clear the content information in @info, keep @URL, @method etc.
 *
 *	No return.
 */
extern void 
http_clear_info(http_info_t *info);


/**
 *	Get the string from @info, the string type is @type.
 *	See macro HTTP_STR_XXXX. 
 *
 *	Return string if success, NULL not found.
 */
extern const char * 
http_get_str(http_info_t *info, int type);


/**
 *	Get the integer value from @info, the type is @type
 *	See macro HTTP_INT_XXXX.
 *
 *	Return regular integer if success, -1 on error.
 */
extern int 
http_get_int(http_info_t *info, int type);


/**
 * 	Get the bool value from @info, the type is @type	
 *	See macro HTTP_BOOL_XXX
 */
extern int
http_get_bool(http_info_t *info, int type);


/**
 *	Get cookie number in @info.
 *
 *	Return the cookie number.
 */
extern int 
http_get_cookie_number(http_info_t *info);


/**
 *	Get the Nth cookie and stored in @name:@value. the Nth is @index.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
http_get_cookie(http_info_t *info, int index, http_cookie_t *cookie);


/**
 *	Get the argument number in @info
 *
 *	Return the argument number.
 */
extern int 
http_get_argument_number(http_info_t *info);


/**
 *	Get the Nth cookie and stored in @name:@value. the Nth is @index.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
http_get_argument(http_info_t *info, int index, http_arg_t *arg);


#endif /* end of HTTP_PROTOCOL_H */
