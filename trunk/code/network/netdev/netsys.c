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

#define	NET_SYS_PATH_LEN	1024

#define	NET_SYS_FMT		"/sys/class/net/%s/device/%s"


static int 
_net_sys_read_int(const char *ifname)
{
	FILE *fp;
	int value = 0;
	int ret = 0;

	fp = open(ifname, "r");
	if (!fp)
		return -1;

	ret = fscanf(fp, "%d", &value);
	if (ret != 1)
		return -1;

	return value;
}

static int 
_net_sys_read_str(const char *ifname, void *buf, size_t len)
{
	FILE *fp;
	int n;

	fp = open(ifname, "r");
	if (!fp)
		return -1;

	memset(buf, 0, len);
	n = fread(buf, len - 1, 1, fp);
	if (n < 0)
		return -1;

	return n;
}

int 
net_sys_get_device_id(const char *ifname)
{
	char fname[NET_SYS_PATH_LEN];

	snprintf(fname, sizeof(fname), NET_SYS_FMT, ifname, "device");
	return _net_sys_get_int(fname);
}

int 
net_sys_get_vendor_id(const char *ifname)
{
	char fname[NET_SYS_PATH_LEN];

	snprintf(fname, sizeof(fname), NET_SYS_FMT, ifname, "vendor");
	return _net_sys_get_int(fname);
}


