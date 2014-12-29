/**
 *	@file	hardware.c
 *
 *	@brief	Init hardware when system startup.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2010-08-30
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>

#include "util.h"


#ifndef LINELEN
#define LINELEN 1024
#endif

/**
 *	Check a PCI device is exist or not exist.
 *
 *	Return 1 if exist, 0 if not exist.
 */
static int 
hw_pcidev_exist(const char *id)
{
	char buf[LINELEN] = {0};
	FILE *fp;
	
	if (!id)
		return 0;
	
	fp = fopen("/proc/pci", "r");
	if (!fp)
		return 0;
	
	while (fgets(buf, LINELEN - 1, fp)) {
		if (strstr(buf, id)) {
			fclose(fp);
			return 1;
		}
	}

	fclose(fp);
	return 0;
}



/**
 *	Get the major number of device @name
 *
 *	Return major number if success, -1 on error.
 */
static int 
hw_dev_major(const char *name)
{
	char buf[LINELEN] = {0};
	FILE *fp = NULL;
	char *ptr = NULL;
	char *begin = NULL;
	char *end = NULL;
	int major = -1;
	
	if (!name)
		return -1;

	fp = fopen("/proc/devices", "r");
	if (!fp) {
		return -1;
	}

	while (fgets(buf, LINELEN - 1, fp)) {
		ptr = strstr(buf, name);
		if (!ptr)
			continue;

		if (isspace(*(ptr - 1)) && isspace(*(ptr + strlen(name)))) 
		{
			begin = buf;
			while (*begin && isspace(*begin))
				++begin;
			end = begin;
			while (*end && isdigit(*end))
				++end;
			if (*end)
				*end = '\0';
			major = atoi(begin);
			fclose(fp);
			return major;
		}

	}

	fclose(fp);

	return major;
}


/**
 *	Init XMLCP card when system startup
 *
 *	Return 0 if success, -1 on error.
 */
int 
hw_init_xmlcp(void)
{
	int cpp_major = 0;
	int cpp_jam_major = 0;
	char buf[512] = {0};

	if (!hw_pcidev_exist("1863:0003"))
		return -1;

	printf("\nXMLCP card initiate...\n");
	
	ut_runcmd("/bin/busybox insmod /modules/cpp.ko");
	ut_runcmd("/bin/busybox insmod /modules/cpp_jam.ko");
	
	cpp_major = hw_dev_major("cpp");
	if (cpp_major > 0) {
		snprintf(buf, 511, 
			 "/bin/busybox mknod -m 0666 /dev/cpp c %d 0", 
			 cpp_major);
		ut_runcmd(buf);
	}
	
	cpp_jam_major = hw_dev_major("cpp_jam");
	if (cpp_jam_major > 0) {
		snprintf(buf, 511, 
			 "/bin/busybox mknod -m 0666 /dev/cpp_jam c %d 0", 
			 cpp_jam_major);
		ut_runcmd(buf);
	}
		
	ut_runcmd("/data/bin/cpp_manager -c 0 0 /data/xmachine_4vlx60ff1148.bit");

	return 0;
}


/**
 *	Init CP7 bypass function when system startup
 *
 *	Return 0 if success, -1 on error.
 */
int 
hw_init_bypass(void)
{
	return 0;
}


