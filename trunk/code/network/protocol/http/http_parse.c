/**
 *	file	http_protocol.c
 *	brief	HTTP protocol APIs
 *
 *	author	Forrest.zhang
 */

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

#include "http_parse.h"


/**
 *	Define some macro to print debug message
 */
#ifdef	_HTTP_DEBUG
#define _HTTP_ERR(fmt, args...)		fprintf(stderr, "http:%s:%d: " fmt, \
						__FILE__, __LINE__, ##args)
#define _HTTP_STATE(fmt, args...)	printf("http: " fmt, ##args)
#else
#define	_HTTP_ERR(fmt, args...)
#define _HTTP_STATE(fmt, args...)
#endif


#define _HTTP_CACHE_CHAR(state, ptr)	     \
	if (state->csize < HTTP_MAX_CACHE - 1) { \
		state->cache[state->csize] = *ptr;	\
		state->csize++;				\
	}


/**
 *	Detect char @c is a separator or not
 *
 *	Return 1 if @c is a separator, else return 0.
 */
static int 
_http_is_SEPARATOR(int c)
{
	if (c == '(' || c == ')' || c == '<' || c == '>' || c == '@' ||
	    c == ',' || c == ';' || c == ':' || c == '\\' || c == '"' ||
	    c == '/' || c == '[' || c == ']' || c == '?' || c == '=' ||
	    c == '{' || c == '}' || c == HTTP_CHR_SP || c == HTTP_CHR_HT)
		return 1;
	else
		return 0;
}


/**
 *	Detect char @c is a valid token char or not.
 *
 *	Return 1 if @c is a valid token char, 0 means not.
 */
static int 
_http_is_TOKEN(int c)
{
	if (c > 31 && c < 127 && !_http_is_SEPARATOR(c))
		return 1;
	else
		return 0;
}


/**
 *	Detect char @c is a valid URL char or not
 *
 *	Return 1 if @c is a valid URL char, 0 means not.
 */
static int
_http_is_URL(int c)
{
	if (c > 31 && c < 127)
		return 1;
	else
		return 0;
}


/**
 *	Detect the content type. the @buf length is @siz.
 *
 *	Return the content type defined in http_protocol.h
 */
static u_int8_t 
_http_CTYPE(const char *buf, size_t siz)
{
	assert(buf);
	assert(siz > 0);

	if (siz >= 9 && strncasecmp(buf, "text/html", 9) == 0) {
		return HTTP_CTE_TEXT_HTML;
	}

	if (siz >= 8 && strncasecmp(buf, "text/xml", 8) == 0) {
		return HTTP_CTE_TEXT_XML;
	}

	if (siz >= 9 && strncasecmp(buf, "text/plain", 10) == 0) {
		return HTTP_CTE_TEXT_PLAIN;
	}

	return HTTP_CTE_UNKOWNED;
}


/**
 *	Alloc memory for @str from @buffer
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_http_alloc_memory(http_buffer_t *b, http_string_t *s)
{
	if (b->start >= HTTP_MAX_BUF)
		return -1;

	s->start = b->start;
	s->len = 0;

	return 0;
}


/**
 *	Alloc memory for @str from buffer, the memory length is @len	
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_http_alloc_memory2(http_buffer_t *b, http_string_t *s, int len)
{
	if ( (b->start + len) >= HTTP_MAX_BUF)
		return -1;

	s->start = b->start;
	s->len = len;
	b->start += len;

	return 0;
}


/**
 *	Copy string @s into @str, the string len is @len.
 *	and @str is malloced by @_http_alloc_memory().
 *	If @end is non-zero, it'll try add '\0' at end of @str.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_http_copy_memory(http_buffer_t *b, http_string_t *s, 
		  const char *str, size_t len, int end)
{
	if (!b || !s || !str || len < 1)
		return -1;

	if (b->start + len >= HTTP_MAX_BUF) {
		_HTTP_ERR("no memory for buffer\n");
		return -1;
	}
	
	if (len < 1)
		return 0;
	
	if (s->start > b->start) {
		_HTTP_ERR("the buffer position have problem\n");
		return -1;
	}

	memcpy(b->buffer + b->start, str, len);

	b->start += len;
	s->len += len;

	if (end && b->start <= HTTP_MAX_BUF) {
		b->buffer[b->start] = 0;
		b->start++;
	}

	return 0;
}


/**
 *	Copy string @s into @str, the string len is @len.
 *	and @str is malloced by @_http_alloc_memory2().
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_http_copy_memory2(http_buffer_t *b, http_string_t *s, 
		  const char *str, size_t len)
{
	memcpy(b->buffer + b->start, str, len);

	return 0;
}


/**
 *	Convert http_string_t value @s to string and return it.
 *
 *	Return string if success, NULL on error.
 */
static char *  
_http_string_ptr(http_buffer_t *b, http_string_t *s)
{
	assert(s);
	assert(b);

	if (s->len < 1)
		return NULL;

	if (s->start >= HTTP_MAX_BUF)
		return NULL;

	return (b->buffer + s->start);
}


static void 
_http_check_METHOD(http_info_t *info)
{
	http_state_t *state;

	assert(info);
	
	state = &info->state;

	switch (state->cache[0]) {
		
	case 'O':

		if (state->csize != 7 || strncmp(state->cache, "OPTIONS", 7))
			info->para.method = HTTP_MED_EXTENSION;
		else
			info->para.method = HTTP_MED_OPTIONS;
		break;

	case 'G':
		
		if (state->csize != 3 || strncmp(state->cache, "GET", 3))
			info->para.method = HTTP_MED_EXTENSION;
		else
			info->para.method = HTTP_MED_GET;
		break;

	case 'H':

		if (state->csize != 4 || strncmp(state->cache, "HEAD", 4))
			info->para.method = HTTP_MED_EXTENSION;
		else
			info->para.method = HTTP_MED_HEAD;

		break;

	case 'P':
		
		if (state->csize == 3 && !strncmp(state->cache, "PUT", 3))
			info->para.method = HTTP_MED_PUT;
		else if (state->csize == 4&&!strncmp(state->cache, "POST", 4))
			info->para.method = HTTP_MED_POST;
		else
			info->para.method = HTTP_MED_EXTENSION;
		break;

	case 'D':

		if (state->csize != 6 || strncmp(state->cache, "DELETE", 6))
			info->para.method = HTTP_MED_EXTENSION;
		else
			info->para.method = HTTP_MED_DELETE;
		break;

	case 'T':

		if (state->csize != 5 || strncmp(state->cache, "TRACE", 5))
			info->para.method = HTTP_MED_EXTENSION;
		else
			info->para.method = HTTP_MED_TRACE;
		break;

	case 'C':

		if (state->csize != 7 || strncmp(state->cache, "CONNECT", 7))
			info->para.method = HTTP_MED_EXTENSION;
		else
			info->para.method = HTTP_MED_CONNECT;
		break;

	default:

		info->para.method = HTTP_MED_EXTENSION;
		break;
	}
}


/** 
 *	Parse HTTP METHOD, we first store token in req->cache, then if 
 *	whole token is stored or token length is exceed cache size, 
 *	stop cache and eat all other token chars until a SP.
 *
 *	Return pointer to char after METHOD, NULL on error
 */
static const char *
_http_METHOD(http_info_t *info, const char *buf, size_t siz)
{
	const char *ptr;
	int remain;
	http_state_t *state;

	assert(info);
	assert(buf);
	assert(siz > 0);

	state = &info->state;

	info->line = 1;
	info->col = 1;
	state->csize = 0;
	ptr = buf;
	remain = siz;
	while (remain) {
		
		switch (state->tstate) {

		case HTTP_TST_BEGIN:

			/* skip space */
			if (*ptr == HTTP_CHR_SP || *ptr == HTTP_CHR_HT)
				break;

			/* is token char */
			if (!_http_is_TOKEN(*ptr)) {
				_HTTP_ERR("[%u:%u]: invalid token char %u\n", 
					  info->line, info->col, *ptr);
				return NULL;
			}

			/* cache char */
			_HTTP_CACHE_CHAR(state, ptr);
			state->tstate = HTTP_TST_IN;

			break;

		case HTTP_TST_IN:

			/* encounter space, means method end */
			if (*ptr == HTTP_CHR_SP) {
				state->tstate = HTTP_TST_FIN;
				break;
			}

			/* check token char */
			if (!_http_is_TOKEN(*ptr)) {
				_HTTP_ERR("[%u:%u]: invalid token char %u\n", 
					  info->line, info->col, *ptr);
				return NULL;
			}

			/* cache char if have cache space */
			_HTTP_CACHE_CHAR(state, ptr);

			break;

		default:
			_HTTP_ERR("invalide token state %d\n", state->tstate);
			return NULL;
		}

		if (state->tstate == HTTP_TST_FIN)
			break;

		remain--;
		ptr++;
		info->pos++;
		info->col++;
	}

	/* token is not finished */
	if (state->tstate != HTTP_TST_FIN)
		return ptr;

	/* decide method */
	state->cache[state->csize] = 0;
	_http_check_METHOD(info);

	_HTTP_STATE("method(%u): %s\n", info->para.method, state->cache);

	/* change state */
	state->pstate = info->state.mstate;
	state->mstate = HTTP_STE_URL;
	state->tstate = HTTP_TST_BEGIN;
	state->csize = 0;
	state->tokpos = 0;

	return ptr;
}


/**
 *	Parse HTTP request URL.
 *
 *	Return pointer to char after URL, NULL on error.
 */
static const char *
_http_URL(http_info_t *info, const char *buf, size_t siz)
{
	const char *ptr;
	const char *begin;
	http_buffer_t *b;
	http_string_t *url;
	int remain;
	size_t ulen = 0;
	http_state_t *state;

	assert(info);
	assert(buf);
	assert(siz > 0);

	state = &info->state;
	b = &info->buffer;
	url = &info->para.oriurl;

	ptr = buf;
	begin = buf;
	remain = siz;
	while (remain) {

		switch (state->tstate) {

		case HTTP_TST_BEGIN:
			
			/* skip start SPACE */
			if (*ptr == HTTP_CHR_SP)
				break;

			/* check first char */
			if (!_http_is_URL(*ptr)) {
				_HTTP_ERR("[%u:%u]: invalid url char %u\n", 
					  info->line, info->col, *ptr);
				return NULL;
			}

			state->tstate = HTTP_TST_IN;
			_http_alloc_memory(b, url);
			begin = ptr;
			
			break;

		case HTTP_TST_IN:
			
			/* end of header line */
			if (*ptr == HTTP_CHR_CR || 
			    *ptr == HTTP_CHR_LF ||
			    *ptr == HTTP_CHR_SP) 
			{
				state->tstate = HTTP_TST_FIN;
			}

			if (*ptr == '?')
				url->have_arg = 1;

			if (*ptr == '%')
				url->is_encoded = 1;

			break;

		default:
			_HTTP_ERR("invalid token state %d\n", state->tstate);
			return NULL;

		}
		
		if (state->tstate == HTTP_TST_FIN)
			break;

		ulen++;
		remain --;
		ptr ++;
		info->pos++;
		info->col++;		
	}

	if (state->tstate != HTTP_TST_FIN) {
		_http_copy_memory(b, url, begin, ulen, 0);
		return ptr;
	}

	_http_copy_memory(b, url, begin, ulen, 1);

	_HTTP_STATE("URL: %s\n", _http_string_ptr(b, url));
	
	state->pstate = state->mstate;
	state->mstate = HTTP_STE_VERSION;
	state->tstate = HTTP_TST_BEGIN;
	state->csize = 0;
	state->tokpos = 0;

	return ptr;
}


/**
 *	Parse HTTP response code.
 *
 *	Return pointer to next char after response code, NULL on error
 */
static const char *
_http_CODE(http_info_t *info, const char *buf, size_t siz)
{
	const char *ptr;
	int remain;
	http_state_t *state;

	assert(info);
	assert(buf);
	assert(siz > 0);
	
	state = &info->state;

	ptr = buf;
	remain = siz;
	while (remain > 0) {
		
		switch (state->tstate) {

		case HTTP_TST_BEGIN:

			if (*ptr == HTTP_CHR_SP || *ptr == HTTP_CHR_HT)
				break;

			if (!isdigit(*ptr)) {
				_HTTP_ERR("[%u:%u]: invalid token char %u\n", 
					  info->line, info->col, *ptr);
				return NULL;
			}

			_HTTP_CACHE_CHAR(state, ptr);
			state->tstate = HTTP_TST_IN;
			state->tokpos = 1;

			break;

		case HTTP_TST_IN:

			if (*ptr == HTTP_CHR_SP && state->tokpos != 4) {
				state->tstate = HTTP_TST_FIN;
				break;
			}

			if (!isdigit(*ptr)) {
				_HTTP_ERR("[%u:%u]: invalid token char %u\n", 
					  info->line, info->col, *ptr);
				return NULL;
			}

			if (state->tokpos > 3) {
				_HTTP_ERR("[%u:%u]: invalid token char %u\n", 
					  info->line, info->col, *ptr);
				return NULL;
			}

			_HTTP_CACHE_CHAR(state, ptr);
			state->tokpos++;

			break;

		default:
			_HTTP_ERR("invalid token state %d\n", state->tstate);
			return NULL;
		}

		ptr++;
		remain--;
		info->pos++;
		info->col++;

		if (state->tstate == HTTP_TST_FIN)
			break;
	}

	if (state->tstate != HTTP_TST_FIN)
		return ptr;

	state->cache[state->csize] = 0;
	info->para.retcode = atoi(state->cache);

	_HTTP_STATE("response code is %d\n", info->para.retcode);

	state->pstate = state->mstate;
	state->mstate = HTTP_STE_REASON;
	state->tstate = HTTP_TST_BEGIN;
	state->csize = 0;
	state->tokpos = 0;

	return ptr;
}


/**
 *	Parse HTTP response reason string
 *
 *	Return next char after reasion token, NULL on error.
 */
static const char *
_http_REASON(http_info_t *info, const char *buf, size_t siz)
{
	const char *ptr;
	int remain;
	http_state_t *state;

	assert(info);
	assert(buf);
	assert(siz > 0);

	state = &info->state;
	
	ptr = buf;
	remain = siz;
	while (remain) {

		switch (state->tstate) {

		case HTTP_TST_BEGIN:
			
			if (*ptr == HTTP_CHR_SP || *ptr == HTTP_CHR_HT)
				break;

			if (*ptr == HTTP_CHR_CR || *ptr == HTTP_CHR_LF) {
				_HTTP_ERR("[%u:%u]: invalid token char %u\n", 
					  info->line, info->col, *ptr);
				return NULL;
			}

			_HTTP_CACHE_CHAR(state, ptr);
			state->tstate = HTTP_TST_IN;			

			break;

		case HTTP_TST_IN:
			
			if (*ptr == HTTP_CHR_CR || *ptr == HTTP_CHR_LF) {
				state->tstate = HTTP_TST_FIN;
				break;
			}

			_HTTP_CACHE_CHAR(state, ptr);

			break;

		default:
			_HTTP_ERR("invalid token state %d\n", state->tstate);
			return NULL;
		}

		/* the last token before CRLF didn't eat CR */
		if (state->tstate == HTTP_TST_FIN)
			break;

		remain --;
		ptr ++;
		info->pos++;
		info->col++;
	}

	if (state->tstate != HTTP_TST_FIN)
		return ptr;

	state->cache[state->csize] = 0;
	_HTTP_STATE("response reason: %s\n", state->cache);

	state->pstate = state->mstate;
	state->mstate = HTTP_STE_CRLF;
	state->tstate = HTTP_TST_BEGIN;
	state->csize = 0;
	state->tokpos = 0;

	return ptr;
}


/**
 *	Parse HTTP version token. if req is not zero, the @buf is a 
 *	request buffer, else it's a response buffer.
 *
 *	Return pointer to char after version token if success, 
 *	NULL on error.
 */
static const char *
_http_VERSION(http_info_t *info, const char *buf, size_t siz)
{
	const char *ptr;
	int remain;
	http_state_t *state;
	u_int8_t *ver;

	assert(info);
	assert(buf);
	assert(siz > 0);
	
	state = &info->state;

	if (info->msg_type == HTTP_REQUEST)
		ver = &info->para.req_version;
	else
		ver = &info->para.res_version;

	info->line = 1;
	info->col = 1;
	ptr = buf;
	remain = siz;
	while (remain > 0) {

		switch (state->tstate) {

		case HTTP_TST_BEGIN:
			
			if (*ptr == HTTP_CHR_SP || *ptr == HTTP_CHR_HT)
				break;

			if (*ptr != 'H') {
				_HTTP_ERR("[%u:%u]: invalid token char %u\n", 
					  info->line, info->col, *ptr);
				return NULL;
			}

			_HTTP_CACHE_CHAR(state, ptr);
			state->tokpos = 0;
			state->tstate = HTTP_TST_IN;
			break;

		case HTTP_TST_IN:

			/* not match "HTTP/" */
			if ( ( (state->tokpos == 1 || state->tokpos == 2) 
			       && *ptr != 'T') 
			     || ( state->tokpos == 3 && *ptr != 'P') 
			     || ( state->tokpos == 4 && *ptr != '/')) 
			{				
				_HTTP_ERR("[%u:%u]: invalid token char %u\n", 
					  info->line, info->col, *ptr);
				return NULL;
			}

			_HTTP_CACHE_CHAR(state, ptr);
			break;
			
		default:
			_HTTP_ERR("invalid token state %d\n", state->tstate);
			return NULL;
		}

		if (state->tstate == HTTP_TST_FIN)
			break;

		ptr++;
		remain--;
		state->tokpos++;
		info->pos++;
		info->col++;
	}
	
	if (state->tstate != HTTP_TST_FIN)
		return ptr;
	
	state->cache[state->csize] = 0;	
	if (state->csize == 8 && state->cache[5] == '1') {
		if (state->cache[7] == '0') {
			*ver = HTTP_VER_10;
		}
		else if (state->cache[7] == '1') {
			*ver = HTTP_VER_11;
		}
		else {
			*ver = HTTP_VER_UNKOWNED;
		}
	}
	else {
		*ver = HTTP_VER_UNKOWNED;
	}

	_HTTP_STATE("version(%d): %s\n", *ver, state->cache);

	state->pstate = state->mstate;
	state->tstate = HTTP_TST_BEGIN;
	state->csize = 0;
	state->tokpos = 0;

	if (info->msg_type == HTTP_REQUEST)
		state->mstate = HTTP_STE_CRLF;
	else
		state->mstate = HTTP_STE_CODE;

	return ptr;
}


/**
 *	Parse CR-LF at end of each header line
 *
 *	Return pointer to char after CR-LF, NULL on error.
 */
static const char *
_http_CRLF(http_info_t *info, const char *buf, size_t siz)
{
	int remain;
	const char *ptr;
	http_state_t *state;
	int version;

	assert(info);
	assert(buf);
	assert(siz > 0);

	state = &info->state;

	if (info->msg_type == HTTP_REQUEST)
		version = info->para.req_version;
	else
		version = info->para.res_version;

	remain = siz;
	ptr = buf;
	while (remain) {
		
		switch (state->tstate) {

		case HTTP_TST_BEGIN:

			if ( *ptr == HTTP_CHR_SP || *ptr == HTTP_CHR_HT)
				break;

			if (*ptr == HTTP_CHR_LF) {
				state->tstate = HTTP_TST_FIN;
				break;
			}

			if ( *ptr != HTTP_CHR_CR) {
				_HTTP_ERR("[%u:%u]: invalid CRLF char %u\n", 
					  info->line, info->col, *ptr);
				return NULL;
			}

			state->tstate = HTTP_TST_IN;
			break;

		case HTTP_TST_IN:
			
			if ( *ptr != HTTP_CHR_LF) {
				_HTTP_ERR("[%u:%u]: invalid CRLF char %i\n", 
					  info->line, info->col, *ptr);
				return NULL;
			}

			state->tstate = HTTP_TST_FIN;
			break;

		default:
			_HTTP_ERR("invalid token state: %d\n", state->tstate);
			return NULL;
		}

		ptr++;
		remain--;
		info->pos++;
		info->col++;

		if (state->tstate == HTTP_TST_FIN)
			break;
	}

	if (state->tstate != HTTP_TST_FIN)
		return ptr;

	_HTTP_STATE("CRLF finished\n");
	
	/* body is start */
	if (state->pstate == HTTP_STE_CRLF) {
		_HTTP_STATE("header finished\n");
		state->mstate = HTTP_STE_BODY;
		if (info->para.blen == 0 && 
		    version == HTTP_VER_11 && 
		    info->para.is_svrclosed == 0) 
		{
			_HTTP_STATE("message finished\n");
			state->mstate = HTTP_STE_FIN;
		}
	}
	else {
		state->pstate = state->mstate;	
		state->mstate = HTTP_STE_HFIELD;
	}

	state->tstate = HTTP_TST_BEGIN;
	state->csize = 0;
	state->tokpos = 0;
	info->col = 1;
	info->line++;

	return ptr;
}


static void 
_http_check_HFILED(http_info_t *info)
{
	http_state_t *state;

	assert(info);
	state = &info->state;

	/* check header field name */
	switch (state->csize) { 

	case 4:
		if (strncasecmp(state->cache, "Host", 4) == 0)
			state->hstate = HTTP_HST_HOST;
		break;
	case 5:
		break;
	case 6:
		if (strncasecmp(state->cache, "Cookie", 6) == 0)
			state->hstate = HTTP_HST_COOKIE;
		break;
	case 7:
		break;
	case 8:
		if (strncasecmp(state->cache, "Loaction", 8) == 0)
			state->hstate = HTTP_HST_LOCATION;
		break;
	case 9:
		if (strncasecmp(state->cache, "Reference", 9) == 0)
			state->hstate = HTTP_HST_REFERENCE;
		break;
	case 10:
		if (strncasecmp(state->cache, "Connection", 10) == 0) 
			state->hstate = HTTP_HST_CONNECTION;
		else if (strncasecmp(state->cache, "Set-Cookie", 10) == 0)
			state->hstate = HTTP_HST_SET_COOKIE;
		else if (strncasecmp(state->cache, "User-Agent", 10) == 0)
			state->hstate = HTTP_HST_USER_AGENT;
		break;
	case 11:
		break;
	case 12:
		if (strncasecmp(state->cache, "Content-Type", 12) == 0) 
			state->hstate = HTTP_HST_CONTENT_TYPE;
		break;
	case 13:
		break;
	case 14:
		if (strncasecmp(state->cache, "Content-Length", 14) == 0) 
			state->hstate = HTTP_HST_CONTENT_LENGTH;
		break;
	case 15:
		if (strncasecmp(state->cache, "X-Forwarded-For", 15) == 0)
			state->hstate = HTTP_HST_XFF;
		break;
	default:
		state->hstate = HTTP_HST_EXTENSION;
		break;
	}
}


/**
 *	Parse HTTP header's field, and store state in @state.
 *
 *	No return.
 */
static const char *
_http_HFIELD(http_info_t *info, const char *buf, size_t siz)
{
	int remain;
	const char *ptr;
	http_state_t *state;

	assert(info);
	assert(buf);
	assert(siz > 0);

	state = &info->state;

	remain = siz;
	ptr = buf;
	while (remain) {
		
		switch (state->tstate) {

		case HTTP_TST_BEGIN:

			/* header finished */
			if (*ptr == HTTP_CHR_CR || *ptr == HTTP_CHR_LF) {
				state->mstate = HTTP_STE_CRLF;
				state->tstate = HTTP_TST_BEGIN;
				return ptr;
			}
			
			/* LWS */
			if (*ptr == HTTP_CHR_SP || *ptr == HTTP_CHR_HT) {
				state->mstate = HTTP_STE_HVALUE;
				state->tstate = HTTP_TST_BEGIN;
				return ptr;
			}

			if (!_http_is_TOKEN(*ptr)) {
				_HTTP_ERR("[%u:%u]: invalid hfield char %u\n",
					  info->line, info->col, *ptr);
				return NULL;
			}
			
			_HTTP_CACHE_CHAR(state, ptr);
			state->tstate = HTTP_TST_IN;

			break;

		case HTTP_TST_IN:

			if (*ptr == HTTP_CHR_CR || 
			    *ptr == HTTP_CHR_LF || 
			    *ptr == ':') 
			{
				state->tstate = HTTP_TST_FIN;
				break;
			}


			if (!_http_is_TOKEN(*ptr)) {
				_HTTP_ERR("[%u:%u]: invalid hfield char %u\n",
					  info->line, info->col, *ptr);
				return NULL;
			}

			_HTTP_CACHE_CHAR(state, ptr);

			break;
			
		default:
			_HTTP_ERR("invalid token state %d\n", state->tstate);
			return NULL;
		}

		if (state->tstate == HTTP_TST_FIN)
			break;

		ptr++;
		remain--;
		info->pos++;
		info->col++;
	}

	if (state->tstate != HTTP_TST_FIN)
		return ptr;

	state->cache[state->csize] = 0;
	_http_check_HFILED(info);
	
	_HTTP_STATE("header field(%d): %s\n", state->hstate, state->cache);

	state->pstate = state->mstate;
	state->mstate = HTTP_STE_HVALUE;
	state->tstate = HTTP_TST_BEGIN;
	state->csize = 0;
	state->tokpos = 0;

	return ptr;
}


static http_string_t *  
_http_get_HVALUE_string(http_info_t *info)
{
	http_state_t *state;
	http_buffer_t *b;
	http_string_t *s;

	assert(info);
	state = &info->state;
	b = &info->buffer;

	switch (state->hstate) {

	case HTTP_HST_HOST:
		s = &info->para.host;
		break;
	case HTTP_HST_REFERENCE:
		s = &info->para.reference;
		break;
	case HTTP_HST_USER_AGENT:
		s = &info->para.agent;
		break;
	case HTTP_HST_LOCATION:
		s = &info->para.location;
		break;
	case HTTP_HST_COOKIE:
	case HTTP_HST_SET_COOKIE:
		s = &info->para.tmp;
		break;
	case HTTP_HST_XFF:
		s = &info->para.xff;
		break;
	default:
		s = NULL;
	}
	
	if (s)
		_http_alloc_memory(b, s);

	return s;
}


static void 
_http_decode_cookie(http_info_t *info, http_string_t *s)
{
	int ncookie;
	http_cookie_s_t *cookie;
	http_string_t *str;
	char *ptr;
	int start;
	int state;
	int len;

	assert(info);
	assert(s);
	assert(info->para.ncookie >= 0);

	ncookie = info->para.ncookie;
	if (ncookie >= HTTP_MAX_COOKIE)
		return;

	str = NULL;
	state = 0;
	len = s->len;
	start = s->start;
	ptr = _http_string_ptr(&info->buffer, s);
	cookie = &info->para.cookies[ncookie];
	while (len > 0) {

		switch (*ptr) {

		case HTTP_CHR_EQUAL:
			if (state == 1) {
				str->len++;
			}
			else {
				*ptr = 0;
				state = 1;
				str = NULL;
			}
			break;

		case HTTP_CHR_HT:
		case HTTP_CHR_SP:
			ncookie++;
			if (ncookie >= HTTP_MAX_COOKIE)
				break;

			cookie = &info->para.cookies[ncookie];
			state = 0;
			*ptr = 0;
			str = NULL;
			break;

		case HTTP_CHR_PERCENT:

			if (str) {
				str->is_encoded = 1;
			}
			break;

		default:

			if (str) {
				str->len++;
				break;
			}

			if (state) {
				str = &cookie->value;
			}
			else {
				str = &cookie->name;
			}
			str->start = start;
			str->len = 1;
			break;
		}

		if (ncookie >= HTTP_MAX_COOKIE) {
			break;
		}

		len--;
		ptr++;
		start++;
	}

	info->para.ncookie = ncookie;
}


static void 
_http_check_HVALUE(http_info_t *info, http_string_t *s)
{
	http_state_t *state;
	u_int8_t *ctype;
	
	assert(info);
	state = &info->state;

	if (info->msg_type == HTTP_REQUEST) {
		ctype = &info->para.req_ctype;
	}
	else {
		ctype = &info->para.res_ctype;
	}

	if (s) {
		_HTTP_STATE("value string is: %s\n", 
			    _http_string_ptr(&info->buffer, s));
	}

	switch(state->hstate) {
		
	case HTTP_HST_CONTENT_LENGTH:

		info->para.blen = atoi(state->cache);
		info->bremain = info->para.blen;
		break;

	case HTTP_HST_CONTENT_TYPE:
		*ctype = _http_CTYPE(state->cache, state->csize);
		break;

	case HTTP_HST_CONNECTION:

		if (state->csize == 5 && 
		    strncasecmp(state->cache, "close", 5) == 0) 
		{
			info->para.is_svrclosed = 1;
		}
		break;
		
	case HTTP_HST_COOKIE:
		_http_decode_cookie(info, s);
		break;
	}
}


/**
 *	Parse HTTP header value.
 *
 *	Return pointer to next char after header value, NULL on error.
 */
static const char *
_http_HVALUE(http_info_t *info, const char *buf, size_t siz)
{
	int remain;
	http_string_t *s;
	http_buffer_t *b;
	http_state_t *state;
	const char *ptr;
	const char *p;
	int n;

	assert(info);
	assert(buf);
	assert(siz > 0);

	state = &info->state;
	b = &info->buffer;

	p = buf;
	n = 0;
	ptr = buf;
	remain = siz;
	while (remain) {
		
		switch (state->tstate) {

		/* token begin */
		case HTTP_TST_BEGIN:
			
			/* no header value, just to CRLF */
			if (*ptr == HTTP_CHR_CR || *ptr == HTTP_CHR_LF) {
				state->pstate = state->mstate;
				state->mstate = HTTP_STE_CRLF;
				state->tstate = HTTP_TST_BEGIN;
				_HTTP_STATE("field have no value\n");
				return ptr;
			}
			
			/* skip lead space and ':' */
			if (*ptr == HTTP_CHR_SP || 
			    *ptr == HTTP_CHR_HT ||
			    *ptr == ':')
				break;

			state->tstate = HTTP_TST_IN;
			s = _http_get_HVALUE_string(info);

			/* cache it */
			_HTTP_CACHE_CHAR(state, ptr);

			p = ptr;
			n = 1;

			/* check quote and slash */
			if (*ptr == HTTP_CHR_QUOTE || *ptr == '\\') {
				n--;
				ptr--;
			}

			break;

		/* in token */
		case HTTP_TST_IN:

			n++;
			switch (*ptr) {

			case HTTP_CHR_QUOTE:

				if (state->inquote && !state->inslash)
					state->inquote = 0;
				else
					state->inquote = 1;
				state->inslash = 0;
				break;

			case '\\':

				if (state->inquote && !state->inslash)
					state->inslash = 1;
				else
					state->inslash = 0;
				break;

			case HTTP_CHR_SP:
			case HTTP_CHR_HT:
				if (!state->inquote) {
					state->tstate = HTTP_TST_LWS;
					_http_copy_memory(b, s, p, n, 0);
					p = NULL;
					n = 0;
				}
				state->inslash = 0;
				break;
				
			case HTTP_CHR_CR:
			case HTTP_CHR_LF:
				
				if (!state->inquote) {
					state->tstate = HTTP_TST_FIN;
					n--;
				}
				}
				state->inslash = 0;
				break;

			default:
				state->inslash = 0;
				break;

			_HTTP_CACHE_CHAR(state, ptr);
			break;

		case HTTP_TST_LWS:

			switch (*ptr) {

			case HTTP_CHR_SP:
			case HTTP_CHR_HT:
				break;

			case HTTP_CHR_CR:
			case HTTP_CHR_LF:
				state->tstate = HTTP_TST_FIN;
				break;

			default:
				state->tstate = HTTP_TST_IN;
				p = ptr;
				n = 1;
			}
		}

		if (state->tstate == HTTP_TST_FIN)
			break;

		ptr++;
		remain--;
		info->pos++;
		info->col++;
	}

	if (state->tstate != HTTP_TST_FIN) {
		_http_copy_memory(b, s, p, n, 0);
		return ptr;
	}
	else {
		_http_copy_memory(b, s, p, n, 1);
	}

	state->cache[state->csize] = 0;
	_HTTP_STATE("value is: %s\n", state->cache);

	_http_check_HVALUE(info, s);
	
	state->pstate = state->mstate;
	state->mstate = HTTP_STE_CRLF;
	state->hstate = HTTP_HST_BEGIN;
	state->tstate = HTTP_TST_BEGIN;
	state->csize = 0;
	state->tokpos = 0;

	return ptr;
}


/**
 *	Parse HTTP body.
 *
 *	Return pointer to next char after body, NONE on error.
 */
static const char *
_http_BODY(http_info_t *info, const char *buf, size_t siz)
{
	const char *ptr;
	
	assert(info);
	assert(buf);
	assert(siz > 0);

	if (info->para.blen > 0) {
		if (info->bremain > siz) {
			info->bremain -= siz;
			ptr = buf + siz;
			info->pos += siz;
			_HTTP_STATE("body have %d bytes not fin\n",
				    info->bremain);
		}
		else {
			ptr = buf + info->bremain;
			info->state.mstate = HTTP_STE_FIN;
			info->bremain = 0;
			info->pos += info->bremain;
			_HTTP_STATE("request finished\n");
		}
	}
	else {
		ptr = buf + siz;
	}

	return ptr;
}


int 
http_parse_request(http_info_t *info, const char *buf, size_t siz, size_t start)
{
	const char *begin, *end;
	int remain;

	if (!info || !buf || siz < 1)
		return -1;

	if (info->msg_type != HTTP_REQUEST) {
		memset(info, 0, sizeof(http_info_t));
		info->msg_type = HTTP_REQUEST;
	}

	if (info->state.mstate != HTTP_STE_FIN && 
	    info->state.mstate != HTTP_STE_BEGIN)
	{
//		_HTTP_STATE("pos %u, start %lu\n", info->pos, start);
	}

	begin = buf;
	end = buf;
	remain = siz;
	while (remain > 0) {

		if (info->state.mstate == HTTP_STE_FIN) {
			memset(info, 0, sizeof(http_info_t));
			info->msg_type = HTTP_REQUEST;
		}

		switch (info->state.mstate) {

		case HTTP_STE_BEGIN:
			info->pos = start;
			info->state.mstate = HTTP_STE_METHOD;
			break;
			
		case HTTP_STE_METHOD:

			end = _http_METHOD(info, begin, remain);
			break;
			
		case HTTP_STE_URL:
			
			end = _http_URL(info, begin, remain);
			break;

		case HTTP_STE_VERSION:
			
			end = _http_VERSION(info, begin, remain);
			break;

		case HTTP_STE_CRLF:

			end = _http_CRLF(info, begin, remain);
			break;

		case HTTP_STE_HFIELD:

			end = _http_HFIELD(info, begin, remain);
			break;

		case HTTP_STE_HVALUE:
			
			end = _http_HVALUE(info, begin, remain);
			break;
			

		case HTTP_STE_BODY:
			
			end = _http_BODY(info, begin, remain);
			break;

		default:
			_HTTP_ERR("invalid state %d\n", info->state.mstate);
			return -1;
		}

		/* error ocurred */
		if (end == NULL)
			return -1;

		begin = end;
		remain = siz - (begin - buf);

		if (info->state.mstate == HTTP_STE_FIN)
			return remain;
	}

	return 0;
}


int 
http_parse_response(http_info_t *info, const char *buf, 
		    size_t siz, size_t start)
{
	const char *begin, *end;
	int remain;

	if (!info || !buf || siz < 1)
		return -1;

	/* init info if need */
	if (info->msg_type != HTTP_RESPONSE) {
		memset(info, 0, sizeof(http_info_t));
		info->msg_type = HTTP_RESPONSE;
	}

	if (info->state.mstate != HTTP_STE_FIN && 
	    info->state.mstate != HTTP_STE_BEGIN) 
	{
		_HTTP_STATE("pos %u, start %lu\n", info->pos, start);
	}

	begin = buf;
	end = buf;
	remain = siz;
	while (remain > 0) {

		if (info->state.mstate == HTTP_STE_FIN) {
			memset(info, 0, sizeof(http_info_t) );
			info->msg_type = HTTP_RESPONSE;
		}

		switch (info->state.mstate) {

		case HTTP_STE_BEGIN:
			
			info->pos = start;
			info->state.mstate = HTTP_STE_VERSION;
			break;
			
		case HTTP_STE_VERSION:

			end = _http_VERSION(info, begin, remain);
			break;
			
		case HTTP_STE_CODE:
			
			end = _http_CODE(info, begin, remain);
			break;

		case HTTP_STE_REASON:
			
			end = _http_REASON(info, begin, remain);
			break;

		case HTTP_STE_CRLF:

			end = _http_CRLF(info, begin, remain);
			break;

		case HTTP_STE_HFIELD:

			end = _http_HFIELD(info, begin, remain);
			break;

		case HTTP_STE_HVALUE:
			
			end = _http_HVALUE(info, begin, remain);
			break;
			

		case HTTP_STE_BODY:
			
			end = _http_BODY(info, begin, remain);
			break;

		default:
			_HTTP_ERR("invalid state %u\n", info->state.mstate);
			return -1;
		}

		if (end == NULL)
			return -1;

		begin = end;
		remain = siz - (begin - buf);

		if (info->state.mstate == HTTP_STE_FIN)
			return remain;
	}

	return 0;
}


const char *
http_code2reason(int status_code)
{
	switch (status_code) {

	/** 2XX code */
	case 200:
		return "OK";
	case 201:
		return "Created";
	case 202:
		return "Accepted";
	case 203:
		return "Non-Authoritative Information";
	case 204:
		return "No Content";
	case 205:
		return "Reset Content";
	case 206:
		return "Partial Content";

	/** 3XX code */
	case 300:
		return "Multiple Choices";
	case 301:
		return "Moved Permanently";
	case 302:
		return "Found";
	case 303:
		return "See Other";
	case 304:
		return "Not Modified";
	case 305:
		return "Use Proxy";
	case 306:
		return "Unused";
	case 307:
		return "Temporary Redirect";

	/** 4xx code */
	case 400:
		return "Bad Request";
	case 401:
		return "Unauthorized";
	case 402:
		return "Payment Required";
	case 403:
		return "Forbidden";
	case 404:
		return "Not Found";
	case 405:
		return "Method Not Allowed";
	case 406:
		return "Not Acceptable";
	case 407:
		return "Proxy Authentication Required";
	case 408:
		return "Request Timeout";
	case 409:
		return "Conflict";

	/** 5xx code */
	case 500:
		return "Internal Server Error";
	case 501:
		return "Not Implemented";
	case 502:
		return "Bad Gateway";
	case 503:
		return "Service Unavailable";
	case 504:
		return "Gateway Timeout";
	case 505:
		return "HTTP version Not Supported";

	default:
		return "Unkown";
	}

	return "Unkown";
}


void 
http_clear_info(http_info_t *info)
{

}


const char * 
http_get_str(http_info_t *info, int type)
{
	if (!info)
		return NULL;
	
	switch (type) {

	case HTTP_STR_URL:
		return _http_string_ptr(&info->buffer, &info->para.oriurl);
		
	case HTTP_STR_DECURL:
		return _http_string_ptr(&info->buffer, &info->para.decurl);

	case HTTP_STR_HOST:
		return _http_string_ptr(&info->buffer, &info->para.host);

	case HTTP_STR_REFERENCE:
		return _http_string_ptr(&info->buffer, &info->para.reference);

	case HTTP_STR_LOCATION:
		return _http_string_ptr(&info->buffer, &info->para.location);

	case HTTP_STR_USER_AGENT:
		return _http_string_ptr(&info->buffer, &info->para.agent);

	default:
		break;
	}

	return NULL;
}


int 
http_get_int(http_info_t *info, int type)
{
	if (!info)
		return -1;

	switch(type) {

	case HTTP_INT_REQVER:
		return info->para.req_version;

	case HTTP_INT_RESVER:
		return info->para.res_version;

	case HTTP_INT_METHOD:
		return info->para.method;

	case HTTP_INT_RETCODE:
		return info->para.retcode;

	default:
		break;
	}

	return -1;
}


int 
http_get_cookie(http_info_t *info, int index, http_cookie_t *cookie)
{
	http_cookie_s_t *cookie1;

	if (!info || !cookie)
		return -1;

	if (index >= info->para.ncookie)
		return -1;

	cookie1 = &info->para.cookies[index];

	cookie->name = _http_string_ptr(&info->buffer, &cookie1->name);
	cookie->value = _http_string_ptr(&info->buffer, &cookie1->value);
	cookie->domain = _http_string_ptr(&info->buffer, &cookie1->domain);
	cookie->path = _http_string_ptr(&info->buffer, &cookie1->path);
	cookie->expire = _http_string_ptr(&info->buffer, &cookie1->expire);

	return 0;
}


int 
http_argument_number(http_info_t *info)
{
	return 0;
}


int 
http_get_argument(http_info_t *info, int index, http_arg_t *arg)
{
	return 0;
}



void 
http_print_info(const http_info_t *info)
{




}

