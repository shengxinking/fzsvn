/**
 *	@file	netsys.c
 *
 *	@brief	Get network device information from "/sys/class/net/<ifname>/" 
 *		directory.
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

#define	NET_SYS_PATH_LEN	1024
#define	NET_SYS_FMT		"/sys/class/net/%s/device/%s"


/**
 *	Read integer from file @ifname
 *
 *
 */
static int 
_net_sys_read_hex(const char *ifile)
{
	FILE *fp;
	int value = 0;
	int ret = 0;

	fp = fopen(ifile, "r");
	if (!fp)
		return -1;

	ret = fscanf(fp, "0x%x", &value);
	if (ret != 1) {
		fclose(fp);
		return -1;
	}
	
	fclose(fp);
	
	return value;
}

/**
 *	Read string from file @ifname. Save string into 
 *	@buf, the @buf length is @len.
 *
 *	Return read string length if success, -1 on error.
 */
static int 
_net_sys_read_str(const char *ifile, void *buf, size_t len)
{
	FILE *fp;
	int n;

	fp = fopen(ifile, "r");
	if (!fp)
		return -1;

	memset(buf, 0, len);
	n = fread(buf, len - 1, 1, fp);
	if (n < 0) {
		fclose(fp);
		return -1;
	}

	fclose(fp);

	return n;
}

int 
net_sys_get_device_id(const char *ifname)
{
	char fname[NET_SYS_PATH_LEN];

	snprintf(fname, sizeof(fname), NET_SYS_FMT, ifname, "device");
	return _net_sys_read_hex(fname);
}

int 
net_sys_get_vendor_id(const char *ifname)
{
	char fname[NET_SYS_PATH_LEN];

	snprintf(fname, sizeof(fname), NET_SYS_FMT, ifname, "vendor");
	return _net_sys_read_hex(fname);
}


