/**
 *	@file	mem_util.c
 *
 *	@brief	Memory utils functions. include total size, \
 *		free size. usage.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2012-07-23
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/sysinfo.h>

#include "sysutil.h"

int 
mem_total(void)
{
	struct sysinfo info;

	if (sysinfo(&info)) {
		return -1;
	}

	return (info.totalram / 1024) / 1024;
}

int   
mem_freed(void)
{
	FILE            *fp; 
        unsigned long   mem_total = 0; 
        unsigned long   mem_free = 0; 
        unsigned long   mem_buffers = 0; 
        unsigned long   mem_cached = 0; 
        char            buf[128] = {0};  
 
        fp = fopen("/proc/meminfo", "r"); 
        if (!fp) { 
                printf("open /proc/meminfo error\n"); 
                return 0; 
        } 
 
        fgets(buf, 127, fp); 
        if (strstr(buf, "MemTotal") == NULL) { 
                printf("/proc/meminfo format error\n"); 
                fclose(fp); 
                return 0; 
        } 
        sscanf(buf, "MemTotal: %lu", &mem_total); 
 
        fgets(buf, 127, fp); 
        if (strstr(buf, "MemFree") == NULL) { 
                printf("/proc/meminfo format error\n"); 
                fclose(fp); 
                return 0; 
        } 
 
        sscanf(buf, "MemFree: %lu", &mem_free); 
 
        fgets(buf, 127, fp); 
        if (strstr(buf, "Buffers") == NULL) { 
                printf("/proc/meminfo format error\n"); 
                fclose(fp); 
                return 0; 
        } 
        sscanf(buf, "Buffers: %lu", &mem_buffers); 
 
        fgets(buf, 127, fp); 
        if (strstr(buf, "Cached") == NULL) { 
                printf("/proc/meminfo format error\n"); 
                fclose(fp); 
                return 0; 
        } 
        sscanf(buf, "Cached: %lu", &mem_cached); 
 
        fclose(fp); 
 
        return (mem_cached + mem_buffers + mem_free) / 1024;
}

int 
mem_used(void)
{
	int total;
	int freed;

	total = mem_total();
	freed = mem_freed();
	
	if (total < 1 || freed < 1)
		return -1;

	return (total - freed);
}

int 
mem_usage(void)
{
	int total = 0;
	int freed = 0;

	total = mem_total();
	freed = mem_freed();

	if (total < 1 || freed < 1)
		return -1;

	return ((total - freed) * 100) / total;
}



