/**
 *	@file	netproc.c
 *
 *	@brief	proc/sys/net file operator.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#define	NETP_IP4_PATH	"/proc/sys/net/ipv4/"
#define	NETP_IP6_PATH	"/proc/sys/net/ipv6/"
#define	NETP_CORE_PATH	"/proc/sys/net/core/"

#define	_NETP_ERR(fmt, args...)	\
	printf("%s:%d: "fmt, __FILE__, __LINE__, ##args)

/**
 *	Read integer from proc file @file and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
static int 
_netp_read_int(const char *file)
{
	FILE *fp;
	char *ret;
	char buf[12] = {0};
	
	fp = fopen(file, "r");
	if (!fp) {
		_NETP_ERR("open %s failed\n", file);
		return -1;
	}

	ret = fgets(buf, sizeof(buf) - 1, fp);

	fclose(fp);

	if (!ret) {
		_NETP_ERR("read %s failed\n", file);
		return -1;
	}

	return atoi(buf);
}

/**
 *	Write integer into proc file @file.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_netp_write_int(const char *file, int val)
{
	FILE *fp;
	char buf[12] = {0};
	size_t len, n;

	fp = fopen(file, "w");
	if (!fp) {
		_NETP_ERR("open %s failed\n", file);
		return -1;
	}

	snprintf(buf, sizeof(buf) - 1, "%d\n", val);

	len = strlen(buf);
	n = fwrite(buf, len, 1, fp);
	fclose(fp);

	if (n != 1) {
		_NETP_ERR("write %s failed\n", file);
		return -1;
	}

	return 0;
}

/**
 *	Read string from proc file @file and saved in @buf, 
 *	@buf length is @len.
 *
 *	Return 0 if success, -1 on error.
 */
static int  
_netp_read_str(const char *file, char *buf, size_t len)
{
	FILE *fp;
	char *ret;
	
	fp = fopen(file, "r");
	if (!fp)
		return -1;

	memset(buf, 0, len);
	ret = fgets(buf, len - 1, fp);
	fclose(fp);

	/* remove tail '\n' */
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = 0;

	if (!ret)
		return -1;

	return 0;
}

/**
 *	Write string @buf into proc file @file, the @buf length 
 *	is @len
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_netp_write_str(const char *file, const char *buf)
{
	FILE *fp;
	size_t len, n;

	fp = fopen(file, "w");
	if (!fp)
		return -1;

	len = strlen(buf);
	n = fwrite(buf, len, 1, fp);
	if (buf[len - 1] != '\n')
		fputc('\n', fp);
	fclose(fp);

	if (n != 1)
		return -1;

	return 0;
}


int 
netp_set_ip4_forward(int val)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_IP4_PATH, "ip_forward");
	
	return _netp_write_int(file, val);
}

int 
netp_get_ip4_forward(void)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_IP4_PATH, "ip_forward");
	
	return _netp_read_int(file);
}

int 
netp_set_ip4_promote_secondaries(const char *ifname, int val)
{
	char file[128] = {0};
	size_t len;

	if (!ifname)
		return -1;

	len = sizeof(file) - 1;
	snprintf(file, len, "%sconf/%s/%s", NETP_IP4_PATH, ifname,
		 "promote_secondaries");
	
	return _netp_write_int(file, val);
}

int 
netp_get_ip4_promote_secondaries(const char *ifname)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%sconf/%s/%s", NETP_IP4_PATH, ifname,
		 "promote_secondaries");
	
	return _netp_read_int(file);
}

int 
netp_set_ip4_local_port_range(const char *range)
{
	char file[128] = {0};
	size_t len;

	if (!range)
		return -1;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_IP4_PATH, "ip_local_port_range");
	
	return _netp_write_str(file, range);
}

int 
netp_get_ip4_local_port_range(char *buf, size_t len)
{
	char file[128] = {0};
	size_t len1;

	if (!buf || len < 1)
		return -1;

	len1 = sizeof(file) - 1;
	snprintf(file, len1, "%s%s", NETP_IP4_PATH, "ip_local_port_range");
	
	return _netp_read_str(file, buf, len);	
}


int 
netp_set_tcp4_mem(const char *range)
{
	char file[128] = {0};
	size_t len;

	if (!range)
		return -1;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_IP4_PATH, "tcp_mem");
	
	return _netp_write_str(file, range);
}

int 
netp_get_tcp4_mem(char *buf, size_t len)
{
	char file[128] = {0};
	size_t len1;

	if (!buf || len < 1)
		return -1;

	len1 = sizeof(file) - 1;
	snprintf(file, len1, "%s%s", NETP_IP4_PATH, "tcp_mem");
	
	return _netp_read_str(file, buf, len);
}

int 
netp_set_tcp4_rmem(const char *range)
{
	char file[128] = {0};
	size_t len;

	if (!range)
		return -1;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_IP4_PATH, "tcp_rmem");
	
	return _netp_write_str(file, range);
}

int 
netp_get_tcp4_rmem(char *buf, size_t len)
{
	char file[128] = {0};
	size_t len1;

	if (!buf || len < 1)
		return -1;

	len1 = sizeof(file) - 1;
	snprintf(file, len1, "%s%s", NETP_IP4_PATH, "tcp_rmem");
	
	return _netp_read_str(file, buf, len);
}

int 
netp_set_tcp4_wmem(const char *range)
{
	char file[128] = {0};
	size_t len;

	if (!range)
		return -1;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_IP4_PATH, "tcp_wmem");
	
	return _netp_write_str(file, range);
}

int 
netp_get_tcp4_wmem(char *buf, size_t len)
{
	char file[128] = {0};
	size_t len1;

	if (!buf || len < 1)
		return -1;

	len1 = sizeof(file) - 1;
	snprintf(file, len1, "%s%s", NETP_IP4_PATH, "tcp_wmem");
	
	return _netp_read_str(file, buf, len);
}


int 
netp_set_tcp4_timestamps(int val)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_IP4_PATH, "tcp_timestamps");
	
	return _netp_write_int(file, val);
}

int 
netp_get_tcp4_timestamps(void)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_IP4_PATH, "tcp_timestamps");
	
	return _netp_read_int(file);
}

int 
netp_set_tcp4_max_syn_backlog(int val)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_IP4_PATH, "tcp_max_syn_backlog");
	
	return _netp_write_int(file, val);
}

int 
netp_get_tcp4_max_syn_backlog(void)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_IP4_PATH, "tcp_max_syn_backlog");
	
	return _netp_read_int(file);
}

int 
netp_set_tcp4_tw_reuse(int val)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_IP4_PATH, "tcp_tw_reuse");
	
	return _netp_write_int(file, val);
}

int 
netp_get_tcp4_tw_reuse(void)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_IP4_PATH, "tcp_tw_reuse");
	
	return _netp_read_int(file);
}

int 
netp_set_tcp4_tw_recycle(int val)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_IP4_PATH, "tcp_tw_recycle");
	
	return _netp_write_int(file, val);
}

int 
netp_get_tcp4_tw_recycle(void)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_IP4_PATH, "tcp_tw_recycle");
	
	return _netp_read_int(file);
}

int 
netp_set_tcp4_tw_max_buckets(int val)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_IP4_PATH, "tcp_max_tw_buckets");
	
	return _netp_write_int(file, val);
}

int 
netp_get_tcp4_tw_max_buckets(void)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_IP4_PATH, "tcp_max_tw_buckets");
	
	return _netp_read_int(file);
}

int 
netp_set_tcp4_fin_timeout(int val)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_IP4_PATH, "tcp_fin_timeout");
	
	return _netp_write_int(file, val);
}

int 
netp_get_tcp4_fin_timeout(void)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_IP4_PATH, "tcp_fin_timeout");
	
	return _netp_read_int(file);
}


int 
netp_set_rmem_default(int val)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_CORE_PATH, "rmem_default");
	
	return _netp_write_int(file, val);
}

int 
netp_get_rmem_default(void)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_CORE_PATH, "rmem_default");
	
	return _netp_read_int(file);
}

int 
netp_set_rmem_max(int val)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_CORE_PATH, "rmem_max");
	
	return _netp_write_int(file, val);
}

int 
netp_get_rmem_max(void)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_CORE_PATH, "rmem_max");
	
	return _netp_read_int(file);
}

int 
netp_set_wmem_default(int val)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_CORE_PATH, "wmem_default");
	
	return _netp_write_int(file, val);
}

int 
netp_get_wmem_default(void)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_CORE_PATH, "wmem_default");
	
	return _netp_read_int(file);
}

int 
netp_set_wmem_max(int val)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_CORE_PATH, "wmem_max");
	
	return _netp_write_int(file, val);
}

int 
netp_get_wmem_max(void)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_CORE_PATH, "wmem_max");
	
	return _netp_read_int(file);
}

int 
netp_set_somaxconn(int val)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_CORE_PATH, "somaxconn");
	
	return _netp_write_int(file, val);
}

int 
netp_get_somaxconn(void)
{
	char file[128] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s%s", NETP_CORE_PATH, "somaxconn");
	
	return _netp_read_int(file);
}

int 
netp_set_ip6_accept_dad(const char *ifname, int val)
{
	char file[128] = {0};
	size_t len;

	if (!ifname)
		return -1;

	len = sizeof(file) - 1;
	snprintf(file, len, "%sconf/%s/%s", NETP_IP6_PATH, ifname,
		 "accept_dad");
	
	return _netp_write_int(file, val);
}

int 
netp_get_ip6_accept_dad(const char *ifname)
{
	char file[128] = {0};
	size_t len;

	if (!ifname)
		return -1;

	len = sizeof(file) - 1;
	snprintf(file, len, "%sconf/%s/%s", NETP_IP6_PATH, ifname,
		 "accept_dad");
	
	return _netp_read_int(file);
}


