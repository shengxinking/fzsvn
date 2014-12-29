/**
 *	@file	sysstatus.c
 *
 *	@brief	The system status: CPU usage, CPU freq, memory usage, 
 *		memory size, disk size, disk usage.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2009-05-22
 */



/**
 *	Get CPU usage of cpu @cpuid, it's get information from
 *	/proc/stat file. if @cpuid is -1, get all cpu information. 
 *	It'll spend @useconds micro-seconds
 *	as interval of 2 times getting data. if @useconds is
 *      zero or negative, using @SYS_CPU_INTERVAL as interval.
 *
 *	Return 0 if success, -1 on error.
 */
int 
sys_get_cpu_usage(sys_cpu_usage_t *usg, int cpuid, u_int32_t useconds)
{
	FILE *fp = NULL;
	int linenum = 1;
	char buf[1024] = {0};
	char *ptr = NULL;
	char cpuname[8];
	u_int64_t utime1 = 0, utime2 = 0;
	u_int64_t ntime1 = 0, ntime2 = 0;
	u_int64_t stime1 = 0, stime2 = 0;
	u_int64_t itime1 = 0, itime2 = 0;
	

	if (!usg)
		return -1;

	if (useconds < 1)
		useconds = SYS_CPU_INTERVAL;

	/* get cpu information first */
	if (cpuid >= 0)
		linenum = cpuid + 2;

	fp = fopen("/proc/stat", "r");
	if (!fp) {
		return -1;
	}
	
	while (linenum > 0) {
		ptr = fgets(buf, 1023, fp);
		if (ptr == NULL) {
			fclose(fp);
			return -1;
		}
		linenum--;
	}

	sscanf(buf, "%s %llu %llu %llu %llu", cpuname, 
	       &utime1, &ntime1, &stime1, &itime1);

	fclose(fp);

	if (cpuid >= 0) {
		char cpuname1[8] = {0};
		snprintf(cpuname1, 7, "cpu%d", cpuid);
		if (strcmp(cpuname, cpuname1)) {
			return -1;
		}
	}

	usleep(useconds);

	/* get cpu information second time */
	if (cpuid >= 0)
		linenum = cpuid + 2;

	fp = fopen("/proc/stat", "r");
	if (!fp) {
		return -1;
	}
	
	while (linenum > 0) {
		ptr = fgets(buf, 1023, fp);
		if (ptr == NULL) {
			fclose(fp);
			return -1;
		}
		linenum --;
	}

	sscanf(buf, "%s %llu %llu %llu %llu", cpuname, 
	       &utime2, &ntime2, &stime2, &itime2);

	fclose(fp);

	usg->utime = (u_int32_t)(utime2 - utime1);
	usg->ntime = (u_int32_t)(ntime2 - ntime1);
	usg->stime = (u_int32_t)(stime2 - stime1);
	usg->itime = (u_int32_t)(itime2 - itime1);
	
	return 0;
}


/**
 *	Get memory usage of system, it's get information from
 *	/proc/meminfo file.
 *
 *	Return 0 if success, -1 on error.
 */
int 
sys_get_mem_usage(sys_mem_usage_t *usg)
{

	return 0;
}


/**
 *	Get disk usage of system, it'll check the disk @dname
 *	is mount or not, if it's mouted, using fstatfs(2), if
 *	@dname is not mounted, return -1.
 *
 *	Return 0 if success, -1 on error or not mounted.
 */
int
sys_get_disk_usage(const char *dname, sys_disk_usage_t *usg)
{

	return 0;
}



