/**
 *	@file	netsys.c
 *
 *	@brief	Get network device information from 
 *		"/sys/class/net/<ifname>/" directory.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#include "net_sysfs.h"

#define	NETS_PATH_LEN		2048
#define NETS_PATH_FMT		"/sys/class/net/%s"
#define	NETS_QUE_FMT		NETS_PATH_FMT"/queues"
#define	NETS_DEV_FMT		NETS_PATH_FMT"/device/%s"
#define NETS_IRQ_FMT		NETS_PATH_FMT"/device/msi_irqs"

#define	_NETS_ERRLEN		2047
static char 			_nets_errbuf[_NETS_ERRLEN + 1];

/**
 *	Read HEX integer from file @ifname
 *
 *	Return >=0 if success, -1 on error.
 */
static int 
_nets_read_hex(const char *file)
{
	FILE *fp;
	int value = 0;
	int ret = 0;

	fp = fopen(file, "r");
	if (!fp) {
		snprintf(_nets_errbuf, _NETS_ERRLEN, 
			 "<%s:%d> open %s readonly failed: %s",
			 __FILE__, __LINE__,
			 file, strerror(errno));
		return -1;
	}

	ret = fscanf(fp, "0x%x", &value);
	fclose(fp);

	if (ret != 1) {
		snprintf(_nets_errbuf, _NETS_ERRLEN, 
			 "<%s:%d> fscanf %s failed: %s",
			 __FILE__, __LINE__,
			 file, strerror(errno));
		return -1;
	}
	
	return value;
}

/**
 *	Read DEC integer from file @ifname
 *
 *	Return >=0 if success, -1 on error.
 */
static int 
_nets_read_int(const char *file)
{
	FILE *fp;
	int value = 0;
	int ret = 0;

	fp = fopen(file, "r");
	if (!fp) {
		snprintf(_nets_errbuf, _NETS_ERRLEN, 
			 "<%s:%d> open %s readonly failed: %s",
			 __FILE__, __LINE__,
			 file, strerror(errno));
		return -1;
	}

	ret = fscanf(fp, "%d", &value);
	fclose(fp);

	if (ret != 1) {
		snprintf(_nets_errbuf, _NETS_ERRLEN, 
			 "<%s:%d> fscanf %s failed: %s",
			 __FILE__, __LINE__,
			 file, strerror(errno));
		return -1;
	}
	
	return value;
}

/**
 *	Read string from file @ifname. Save string into 
 *	@buf, the @buf length is @len.
 *
 *	Return read string length if success, -1 on error.
 */
static int 
_nets_read_str(const char *file, char *buf, size_t len)
{
	FILE *fp;
	int n;

	fp = fopen(file, "r");
	if (!fp) {
		snprintf(_nets_errbuf, _NETS_ERRLEN, 
			 "<%s:%d> open %s readonly failed: %s",
			 __FILE__, __LINE__,
			 file, strerror(errno));
		return -1;
	}

	memset(buf, 0, len);
	n = fscanf(fp, "%s", buf);
	fclose(fp);

	if (n != 1) {
		snprintf(_nets_errbuf, _NETS_ERRLEN, 
			 "<%s:%d> fscanf %s failed: %s",
			 __FILE__, __LINE__,
			 file, strerror(errno));
		return -1;
	}

	return 0;
}

/**
 *	Write string @buf to file @ifname. the @buf length is @len.
 *
 *	Return read string length if success, -1 on error.
 */
static int 
_nets_write_str(const char *file, char *buf, size_t len)
{
	FILE *fp;
	int n;

	fp = fopen(file, "w");
	if (!fp) {
		snprintf(_nets_errbuf, _NETS_ERRLEN, 
			 "<%s:%d> open %s writeonly failed: %s",
			 __FILE__, __LINE__,
			 file, strerror(errno));
		return -1;
	}

	n = fwrite(buf, len, 1, fp);
	fclose(fp);
	if (n < 0) {
		snprintf(_nets_errbuf, _NETS_ERRLEN, 
			 "<%s:%d> fwrite(%s) %s failed: %s",
			 __FILE__, __LINE__,
			 buf, file, strerror(errno));
		return -1;
	}

	return 0;
}

const char * 
nets_get_error(void)
{
	return _nets_errbuf;
}

int 
nets_is_exist(const char *ifname)
{
	char fname[NETS_PATH_LEN];
	struct stat st;

	if (!ifname) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> invalid argument",
			 __FILE__, __LINE__);
		return -1;
	}

	snprintf(fname, sizeof(fname), NETS_PATH_FMT, ifname);

	/* get stat failed */
	if (stat(fname, &st))
		return 0;

	if (S_ISDIR(st.st_mode))
		return 1;

	return 0;
}

int 
nets_get_flags(const char *ifname)
{
	char fname[NETS_PATH_LEN];

	if (!ifname) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> invalid argument",
			 __FILE__, __LINE__);
		return -1;
	}

	snprintf(fname, sizeof(fname), NETS_PATH_FMT"/flags", ifname); 
	return _nets_read_hex(fname);
}

int 
nets_get_device_id(const char *ifname)
{
	char fname[NETS_PATH_LEN];
	
	if (!ifname) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> invalid argument",
			 __FILE__, __LINE__);
		return -1;
	}

	snprintf(fname, sizeof(fname), NETS_DEV_FMT, ifname, "device");
	return _nets_read_hex(fname);
}

int 
nets_get_vendor_id(const char *ifname)
{
	char fname[NETS_PATH_LEN];

	if (!ifname) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> invalid argument",
			 __FILE__, __LINE__);
		return -1;
	}

	snprintf(fname, sizeof(fname), NETS_DEV_FMT, ifname, "vendor");
	return _nets_read_hex(fname);
}

int 
nets_get_carrier(const char *ifname)
{
	char fname[NETS_PATH_LEN];

	if (!ifname) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> invalid argument",
			 __FILE__, __LINE__);
		return -1;
	}

	snprintf(fname, sizeof(fname), NETS_PATH_FMT"/carrier", ifname); 
	return _nets_read_int(fname);
}

int 
nets_get_index(const char *ifname)
{
	char fname[NETS_PATH_LEN];

	if (!ifname) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> invalid argument",
			 __FILE__, __LINE__);
		return -1;
	}

	snprintf(fname, sizeof(fname), NETS_PATH_FMT"/ifindex", ifname); 
	return _nets_read_int(fname);
}

char *  
nets_get_hwaddr(const char *ifname, char *addr, size_t len)
{
	char fname[NETS_PATH_LEN];

	if (!ifname || !addr || len < 18) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> invalid argument",
			 __FILE__, __LINE__);
		return NULL;
	}

	snprintf(fname, sizeof(fname), NETS_PATH_FMT"/address", ifname);
	if (_nets_read_str(fname, addr, len))
		return NULL;
	
	return addr;
}

int 
nets_get_mtu(const char *ifname)
{
	char fname[NETS_PATH_LEN];

	if (!ifname) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> invalid argument",
			 __FILE__, __LINE__);
		return -1;
	}

	snprintf(fname, sizeof(fname), NETS_PATH_FMT"/mtu", ifname);
	return _nets_read_int(fname);
}

int 
nets_get_speed(const char *ifname)
{
	char fname[NETS_PATH_LEN];

	if (!ifname) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> invalid argument",
			 __FILE__, __LINE__);
		return -1;
	}

	snprintf(fname, sizeof(fname), NETS_PATH_FMT"/speed", ifname);
	return _nets_read_int(fname);
}

static void 
_nets_free_dirent(struct dirent **dir, int n)
{
	int i;

	if (!dir)
		return;

	for (i = 0; i < n; i++)
		free(dir[i]);
	
	free(dir);	     
}

static int 
_nets_irq_filter(const struct dirent *dir)
{
	if (!dir)
		return 0;

	if (strcmp(dir->d_name, ".") == 0)
		return 0;

	if (strcmp(dir->d_name, "..") == 0)
		return 0;
	
	return 1;
}


int 
nets_get_irq_count(const char *ifname)
{
	char fname[NETS_PATH_LEN];
	struct dirent **dir = NULL;
	int n;

	if (!ifname) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> invalid argument",
			 __FILE__, __LINE__);
		return -1;
	}

	snprintf(fname, sizeof(fname), NETS_IRQ_FMT, ifname);
	n = scandir(fname, &dir, _nets_irq_filter, NULL);
	if (n <= 0)
		return -1;

	_nets_free_dirent(dir, n);

	return n;
}

int 
nets_get_irqs(const char *ifname, int *irqs, int nirq)
{
	char fname[NETS_PATH_LEN];
	struct dirent **dir = NULL;
	int n;
	int i;

	if (!ifname || !irqs || nirq < 1) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> invalid argument",
			 __FILE__, __LINE__);
		return -1;
	}

	snprintf(fname, sizeof(fname), NETS_IRQ_FMT, ifname);
	n = scandir(fname, &dir, _nets_irq_filter, NULL);
	if (n <= 0) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> scandir %s failed: %s",
			 __FILE__, __LINE__, 
			 fname, strerror(errno));
		return -1;
	}

	if (nirq < n) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> smaller irq array %d to store %d irqs",
			 __FILE__, __LINE__, 
			 nirq, n);
		_nets_free_dirent(dir, n);
		return -1;
	}

	/* save IRQ number into @irqs */
	memset(irqs, 0, sizeof(int) * nirq);
	for (i = 0; i < n; i++)
		irqs[i] = atoi(dir[i]->d_name);

	_nets_free_dirent(dir, n);

	return n;
}

static int 
_nets_rps_filter(const struct dirent *dir)
{
	if (!dir)
		return 0;

	if (strcmp(dir->d_name, ".") == 0)
		return 0;

	if (strcmp(dir->d_name, "..") == 0)
		return 0;

	/* only return "rx-N" for RPS */
	if (strncmp(dir->d_name, "rx-", 3))
		return 0;

	return 1;
}

int 
nets_get_rps_count(const char *ifname)
{
	char dname[NETS_PATH_LEN];
	struct dirent **dir = NULL;
	int n;

	if (!ifname) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> invalid argument",
			 __FILE__, __LINE__);
		return -1;
	}

	snprintf(dname, sizeof(dname), NETS_QUE_FMT, ifname);
	n = scandir(dname, &dir, _nets_rps_filter, NULL);
	if (n <= 0) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> scandir %s failed: %s",
			 __FILE__, __LINE__, 
			 dname, strerror(errno));
		return -1;
	}

	_nets_free_dirent(dir, n);
	
	return n;
}

int 
nets_get_rps_cpu(const char *ifname, u_int64_t *masks, int nmask)
{
	char dname[NETS_PATH_LEN];
	char fname[NETS_PATH_LEN];
	struct dirent **dir = NULL;
	char buf[32];
	char *ptr;
	u_int64_t mask;
	u_int64_t lower_mask, higher_mask;
	int ret = 0;
	int n;
	int i;

	if (!ifname || !masks || nmask < 1) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> invalid argument",
			 __FILE__, __LINE__);
		return -1;
	}

	snprintf(dname, sizeof(dname), NETS_QUE_FMT, ifname);
	n = scandir(dname, &dir, _nets_rps_filter, NULL);
	if (n <= 0) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> scandir %s failed: %s",
			 __FILE__, __LINE__,
			 dname, strerror(errno));
		return -1;
	}

	if (nmask < n) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> smaller cpu array %d to store %d cpus",
			 __FILE__, __LINE__, 
			 nmask, n);
		_nets_free_dirent(dir, n);
		return -1;
	}

	/* save cpu flags into @masks */
	memset(masks, 0, sizeof(u_int64_t) * nmask);
	for (i = 0; i < n; i++) {
		snprintf(fname, sizeof(fname), "%s/%s/rps_cpus", 
			 dname, dir[i]->d_name);
		if (_nets_read_str(fname, buf, sizeof(buf))) {
			ret = -1;
			break;
		}

		ptr = strchr(buf, ',');
		if (ptr) {
			sscanf(buf, "%lx,%lx", &higher_mask, &lower_mask);
			mask = lower_mask + (higher_mask << 32);
		}
		else {
			sscanf(buf, "%lx", &mask);
		}
		masks[i] = mask;
	}

	_nets_free_dirent(dir, n);

	if (ret < 0)
		return ret;
	else
		return n;
}

int 
nets_set_rps_cpu(const char *ifname, u_int64_t *masks, int nmask)
{
	char dname[NETS_PATH_LEN];
	char fname[NETS_PATH_LEN];
	struct dirent **dir = NULL;
	char buf[32];
	u_int64_t mask;
	u_int64_t lower_mask, higher_mask;
	int ret = 0;
	int n;
	int i;

	if (!ifname || !masks || nmask < 1) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> invalid argument",
			 __FILE__, __LINE__);
		return -1;
	}

	snprintf(dname, sizeof(dname), NETS_QUE_FMT, ifname);
	n = scandir(dname, &dir, _nets_rps_filter, NULL);
	if (n <= 0) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> scandir %s failed: %s",
			 __FILE__, __LINE__, 
			 dname, strerror(errno));
		return -1;
	}

	if (nmask < n) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> smaller cpu array %d to set %d cpus",
			 __FILE__, __LINE__, 
			 nmask, n);
		_nets_free_dirent(dir, n);
		return -1;
	}

	/* save cpu flags into @masks */
	for (i = 0; i < n; i++) {
		snprintf(fname, sizeof(fname), "%s/%s/rps_cpus", 
			 dname, dir[i]->d_name);
		mask = masks[i];
		memset(buf, 0, sizeof(buf));

		/* only use 32 cpus */
		lower_mask = mask & 0xffffffff;
		higher_mask = mask >> 32;
		if (higher_mask == 0) {
			snprintf(buf, sizeof(buf) - 1, "%lx", lower_mask);
		}
		else {
			snprintf(buf, sizeof(buf) - 1, "%lx, %lx", 
				 higher_mask, lower_mask);
		}

		ret = _nets_write_str(fname, buf, strlen(buf));
		if (ret < 0)
			break;
	}

	_nets_free_dirent(dir, n);

	if (ret < 0)
		return ret;
	else
		return 0;
}

int 
nets_get_rps_flow(const char *ifname, int *flows, int nflow)
{
	char dname[NETS_PATH_LEN];
	char fname[NETS_PATH_LEN];
	struct dirent **dir = NULL;
	int flow;
	int ret = 0;
	int n;
	int i;

	if (!ifname || !flows || nflow < 1) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> invalid argument",
			 __FILE__, __LINE__);
		return -1;
	}

	snprintf(dname, sizeof(dname), NETS_QUE_FMT, ifname);
	n = scandir(dname, &dir, _nets_rps_filter, NULL);
	if (n <= 0) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> scandir %s failed: %s",
			 __FILE__, __LINE__, 
			 dname, strerror(errno));
		return -1;
	}

	if (nflow < n) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> smaller flow array %d to store %d flows",
			 __FILE__, __LINE__, 
			 nflow, n);
		_nets_free_dirent(dir, n);
		return -1;
	}

	/* save cpu flags into @masks */
	memset(flows, 0, sizeof(int) * nflow);
	for (i = 0; i < n; i++) {
		snprintf(fname, sizeof(fname), "%s/%s/rps_flow_cnt", 
			 dname, dir[i]->d_name);
		flow = _nets_read_int(fname);
		if (flow < 0) {
			ret = -1;
			break;
		}
		flows[i] = flow;
	}

	_nets_free_dirent(dir, n);

	if (ret < 0)
		return ret;
	else
		return n;
}

int 
nets_set_rps_flow(const char *ifname, int *flows, int nflow)
{
	char dname[NETS_PATH_LEN];
	char fname[NETS_PATH_LEN];
	struct dirent **dir = NULL;
	char buf[32];
	int ret = 0;
	int n;
	int i;

	if (!ifname || !flows || nflow < 1) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> invalid argument",
			 __FILE__, __LINE__);
		return -1;
	}

	snprintf(dname, sizeof(dname), NETS_QUE_FMT, ifname);
	n = scandir(dname, &dir, _nets_rps_filter, NULL);
	if (n <= 0) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> scandir %s failed: %s",
			 __FILE__, __LINE__, 
			 dname, strerror(errno));
		return -1;
	}

	if (nflow < n) {
		snprintf(_nets_errbuf, _NETS_ERRLEN,
			 "<%s:%d> smaller flow array %d to set %d flows",
			 __FILE__, __LINE__, 
			 nflow, n);
		_nets_free_dirent(dir, n);
		return -1;
	}

	/* save cpu flags into @masks */
	for (i = 0; i < n; i++) {
		snprintf(fname, sizeof(fname), "%s/%s/rps_flow_cnt", 
			 dname, dir[i]->d_name);
		memset(buf, 0, sizeof(buf));

		snprintf(buf, sizeof(buf) - 1, "%d", flows[i]);
		ret = _nets_write_str(fname, buf, strlen(buf));
		if (ret < 0) {
			break;
		}
	}

	_nets_free_dirent(dir, n);

	if (ret < 0)
		return ret;
	else
		return 0;
}


