/**
 *	@file	net_procfs.c
 *
 *	@brief	Get/Set network option by /proc filesystem.
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
#include <errno.h>
#include <dirent.h>

#include "net_procfs.h"

#define	NETP_PATH_LEN		2048
#define	NETP_IP4_PATH		"/proc/sys/net/ipv4"
#define NETP_IP4_CONF_PATH	NETP_IP4_PATH"/conf"
#define	NETP_IP6_PATH		"/proc/sys/net/ipv6"
#define NETP_IP6_CONF_PATH	NETP_IP6_PATH"/conf"
#define	NETP_CORE_PATH		"/proc/sys/net/core"
#define NETP_IRQ_PATH		"/proc/irq"

#define _NETP_ERRLEN		2047
static char			_netp_errbuf[_NETP_ERRLEN + 1];

/**
 *	Read integer from proc file @file and return it.
 *
 *	Return >= 0 if success, -1 on error.
 */
static int 
_netp_read_int(const char *file)
{
	FILE *fp;
	int value;
	int n;
	
	fp = fopen(file, "r");
	if (!fp) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> open %s readonly failed: %s",
			 __FILE__, __LINE__,
			 file, strerror(errno));
		return -1;
	}

	n = fscanf(fp, "%d", &value);
	fclose(fp);

	if (n != 1) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> fscanf %s failed: %s", 
			 __FILE__, __LINE__,
			 file, strerror(errno));
		return -1;
	}

	return value;
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
	char buf[32] = {0};
	size_t len, n;

	fp = fopen(file, "w");
	if (!fp) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> open %s writeonly failed: %s",
			 __FILE__, __LINE__,
			 file, strerror(errno));
		return -1;
	}

	snprintf(buf, sizeof(buf) - 1, "%d\n", val);

	len = strlen(buf);
	n = fwrite(buf, len, 1, fp);
	fclose(fp);

	if (n != 1) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> fwrite(%d) %s failed: %s", 
			 __FILE__, __LINE__,
			 val, file, strerror(errno));
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
	if (!fp) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> open %s readonly failed: %s",
			 __FILE__, __LINE__,
			 file, strerror(errno));
		return -1;
	}

	memset(buf, 0, len);
	ret = fgets(buf, len - 1, fp);
	fclose(fp);

	/* remove tail '\n' */
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = 0;

	if (!ret) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> fgets %s failed: %s", 
			 __FILE__, __LINE__,
			 file, strerror(errno));
		return -1;
	}

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
	if (!fp) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> open %s writeonly failed: %s",
			 __FILE__, __LINE__,
			 file, strerror(errno));
		return -1;
	}

	len = strlen(buf);
	n = fwrite(buf, len, 1, fp);
	fclose(fp);
	
	if (n != 1) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> fwrite(%s) %s failed: %s", 
			 __FILE__, __LINE__,
			 buf, file, strerror(errno));
		printf("%s\n", _netp_errbuf);
		return -1;
	}

	return 0;
}

const char * 
netp_get_error(void)
{
	return _netp_errbuf;
}

int 
netp_core_get_rmem_default(void)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/rmem_default", NETP_CORE_PATH);
	
	return _netp_read_int(file);
}

int 
netp_core_set_rmem_default(int val)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/rmem_default", NETP_CORE_PATH);
	
	return _netp_write_int(file, val);
}

int 
netp_core_get_rmem_max(void)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/rmem_max", NETP_CORE_PATH);
	
	return _netp_read_int(file);
}

int 
netp_core_set_rmem_max(int val)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/rmem_max", NETP_CORE_PATH);
	
	return _netp_write_int(file, val);
}

int 
netp_core_get_wmem_default(void)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/wmem_default", NETP_CORE_PATH);
	
	return _netp_read_int(file);
}

int 
netp_core_set_wmem_default(int val)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/wmem_default", NETP_CORE_PATH);
	
	return _netp_write_int(file, val);
}

int 
netp_core_get_wmem_max(void)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/wmem_max", NETP_CORE_PATH);
	
	return _netp_read_int(file);
}

int 
netp_core_set_wmem_max(int val)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/wmem_max", NETP_CORE_PATH);
	
	return _netp_write_int(file, val);
}

int 
netp_core_get_somaxconn(void)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/somaxconn", NETP_CORE_PATH);
	
	return _netp_read_int(file);
}

int 
netp_core_set_somaxconn(int val)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/somaxconn", NETP_CORE_PATH);
	
	return _netp_write_int(file, val);
}

int 
netp_core_get_rps(void)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/rps_sock_flow_entries", NETP_CORE_PATH);
	
	return _netp_read_int(file);
}

int 
netp_core_set_rps(int val)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/rps_sock_flow_entries", NETP_CORE_PATH);
	
	return _netp_write_int(file, val);
}

int 
netp_ip4_get_forward(void)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/ip_forward", NETP_IP4_PATH);
	
	return _netp_read_int(file);
}

int 
netp_ip4_set_forward(int val)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/ip_forward", NETP_IP4_PATH);
	
	return _netp_write_int(file, val);
}

char *  
netp_ip4_get_local_port_range(char *buf, size_t len)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len1;

	if (!buf || len < 1)
		return NULL;

	len1 = sizeof(file) - 1;
	snprintf(file, len1, "%s/ip_local_port_range", NETP_IP4_PATH);
	if (_netp_read_str(file, buf, len) < 0)
		return NULL;

	return buf;
}

int 
netp_ip4_set_local_port_range(const char *range)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	if (!range)
		return -1;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/ip_local_port_range", NETP_IP4_PATH);
	
	return _netp_write_str(file, range);
}

int 
netp_ip4_get_promote_secondaries(const char *ifname)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	if (!ifname) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/%s/promote_secondaries", 
		 NETP_IP4_CONF_PATH, ifname);
	
	return _netp_read_int(file);
}

int 
netp_ip4_set_promote_secondaries(const char *ifname, int val)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	if (!ifname) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/%s/promote_secondaries", 
		 NETP_IP4_CONF_PATH, ifname);
	
	return _netp_write_int(file, val);
}

int 
netp_ip6_get_accept_dad(const char *ifname)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	if (!ifname) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/%s/accept_dad", NETP_IP6_CONF_PATH, ifname); 
	
	return _netp_read_int(file);
}

int 
netp_ip6_set_accept_dad(const char *ifname, int val)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	if (!ifname) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/%s/accept_dad", NETP_IP6_CONF_PATH, ifname);
	
	return _netp_write_int(file, val);
}

int 
netp_ip6_get_disable(const char *ifname)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	if (!ifname) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/%s/disable", NETP_IP6_CONF_PATH, ifname);
	
	return _netp_read_int(file);
}

int 
netp_ip6_set_disable(const char *ifname, int val)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	if (!ifname) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/%s/disable", NETP_IP6_CONF_PATH, ifname);
	
	return _netp_write_int(file, val);
}

int 
netp_ip6_get_forwarding(const char *ifname)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	if (!ifname) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/%s/forwarding", NETP_IP6_CONF_PATH, ifname);
	
	return _netp_read_int(file);
}

int 
netp_ip6_set_forwarding(const char *ifname, int val)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	if (!ifname) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/%s/forwarding", NETP_IP6_CONF_PATH, ifname);
	
	return _netp_write_int(file, val);
}

int 
netp_ip6_get_mtu(const char *ifname)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	if (!ifname) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/%s/mtu", NETP_IP6_CONF_PATH, ifname);
	
	return _netp_read_int(file);
}

int 
netp_ip6_set_mtu(const char *ifname, int val)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	if (!ifname) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/%s/mtu", NETP_IP6_CONF_PATH, ifname);
	
	return _netp_write_int(file, val);
}

char * 
netp_tcp4_get_mem(char *buf, size_t len)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len1;

	if (!buf || len < 1) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return NULL;
	}

	len1 = sizeof(file) - 1;
	snprintf(file, len1, "%s/tcp_mem", NETP_IP4_PATH);
	
	if (_netp_read_str(file, buf, len) < 0)
		return NULL;

	return buf;
}

int 
netp_tcp4_set_mem(const char *range)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	if (!range) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/tcp_mem", NETP_IP4_PATH);
	
	return _netp_write_str(file, range);
}

char * 
netp_tcp4_get_rmem(char *buf, size_t len)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len1;

	if (!buf || len < 1) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return NULL;
	}

	len1 = sizeof(file) - 1;
	snprintf(file, len1, "%s/tcp_rmem", NETP_IP4_PATH);

	if (_netp_read_str(file, buf, len) < 0)
		return NULL;

	return buf;
}

int 
netp_tcp4_set_rmem(const char *range)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	if (!range) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/tcp_rmem", NETP_IP4_PATH);
	
	return _netp_write_str(file, range);
}

char *  
netp_tcp4_get_wmem(char *buf, size_t len)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len1;

	if (!buf || len < 1) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return NULL;
	}

	len1 = sizeof(file) - 1;
	snprintf(file, len1, "%s/tcp_wmem", NETP_IP4_PATH);
	
	if (_netp_read_str(file, buf, len) < 0)
		return NULL;

	return buf;
}

int 
netp_tcp4_set_wmem(const char *range)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	if (!range) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> invalid argument", 
			 __FILE__, __LINE__);
		return -1;
	}

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/tcp_wmem", NETP_IP4_PATH);
	
	return _netp_write_str(file, range);
}

int 
netp_tcp4_get_timestamps(void)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/tcp_timestamps", NETP_IP4_PATH);
	
	return _netp_read_int(file);
}

int 
netp_tcp4_set_timestamps(int val)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/tcp_timestamps", NETP_IP4_PATH);
	
	return _netp_write_int(file, val);
}

int 
netp_tcp4_get_max_syn_backlog(void)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/tcp_max_syn_backlog", NETP_IP4_PATH);
	
	return _netp_read_int(file);
}

int 
netp_tcp4_set_max_syn_backlog(int val)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/tcp_max_syn_backlog", NETP_IP4_PATH);
	
	return _netp_write_int(file, val);
}

int 
netp_tcp4_get_tw_reuse(void)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/tcp_tw_reuse", NETP_IP4_PATH);
	
	return _netp_read_int(file);
}

int 
netp_tcp4_set_tw_reuse(int val)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/tcp_tw_reuse", NETP_IP4_PATH);
	
	return _netp_write_int(file, val);
}

int 
netp_tcp4_get_tw_recycle(void)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/tcp_tw_recycle", NETP_IP4_PATH);
	
	return _netp_read_int(file);
}

int 
netp_tcp4_set_tw_recycle(int val)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/tcp_tw_recycle", NETP_IP4_PATH);
	
	return _netp_write_int(file, val);
}

int 
netp_tcp4_get_max_tw_buckets(void)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/tcp_max_tw_buckets", NETP_IP4_PATH);
	
	return _netp_read_int(file);
}

int 
netp_tcp4_set_max_tw_buckets(int val)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/tcp_max_tw_buckets", NETP_IP4_PATH);
	
	return _netp_write_int(file, val);
}

int 
netp_tcp4_get_fin_timeout(void)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/tcp_fin_timeout", NETP_IP4_PATH);
	
	return _netp_read_int(file);
}

int 
netp_tcp4_set_fin_timeout(int val)
{
	char file[NETP_PATH_LEN] = {0};
	size_t len;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/tcp_fin_timeout", NETP_IP4_PATH);
	
	return _netp_write_int(file, val);
}

u_int32_t  
netp_irq_get_cpu(int irq)
{
	char file[NETP_PATH_LEN] = {0};
	char buf[32];
	u_int32_t mask = 0;
	int n;
        size_t len;
	
	if (irq < 0)
		return 0;

	len = sizeof(file) - 1;
	snprintf(file, len, "%s/%d/smp_affinity", NETP_IRQ_PATH, irq);

	if (_netp_read_str(file, buf, sizeof(buf)))
		return 0;

	n = sscanf(buf, "%x", &mask);
	if (n != 1)
		return 0;

	return mask;
}

int 
netp_irq_set_cpu(int irq, u_int32_t mask)
{
	char file[NETP_PATH_LEN] = {0};
	char buf[32];
        size_t len;
	
	if (irq < 0)
		return -1;

        len = sizeof(file) - 1;
        snprintf(file, len, "%s/%d/smp_affinity", NETP_IRQ_PATH, irq);
	
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf) - 1, "%x", mask);

	return _netp_write_str(file, buf);
}

static void 
_netp_free_dirent(struct dirent **dirs, int n)
{
	int i;

        if (!dirs)
                return;

        for (i = 0; i < n; i++)
                free(dirs[i]);

        free(dirs);
}

static int 
_netp_irq_filter(const struct dirent *dir)
{
	if (!dir)
		return 0;

	if (strcmp(dir->d_name, ".") == 0)
		return 0;

	if (strcmp(dir->d_name, "..") == 0)
		return 0;

	if (dir->d_type & DT_DIR)
		return 1;

	return 0;
}

char * 
netp_irq_get_name(int irq, char *buf, size_t len)
{
	char path[NETP_PATH_LEN] = {0};
	struct dirent **dirs = NULL;
	int n;
        size_t flen;
	
	if (irq < 0 || !buf || len < 1)
		return NULL;

        flen = sizeof(path) - 1;
        snprintf(path, flen, "%s/%d/", NETP_IRQ_PATH, irq);

	n = scandir(path, &dirs, _netp_irq_filter, NULL);
	if (n < 0) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> scandir %s failed: %s",
			 __FILE__, __LINE__,
			 path, strerror(errno));
		return NULL;
	}
	
	if (n != 1) {
		snprintf(_netp_errbuf, _NETP_ERRLEN, 
			 "<%s:%d> scandir %s get %d directorys",
			 __FILE__, __LINE__,
			 path, n);
		_netp_free_dirent(dirs, n);
		return NULL;
	}
	
	memset(buf, 0, len);
	strncpy(buf, dirs[0]->d_name, len - 1);

	_netp_free_dirent(dirs, n);

	return buf;
}


