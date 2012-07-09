/**
 *	@file	cpu_usage.c
 *
 *	@brief	print the cpu usage in 1 second.
 *
 *	@author	Forrest.zhang
 *	
 *	@date
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>

static int _g_ncpus = 1;

typedef struct cpu_usage {
	char		name[16];
	long long	user;
	long long	nice;
	long long	kern;
	long long	idle;
} cpu_usage_t;

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("cpu_usage: show the cpu usage\n");
	printf("\t-n\tthe cpu count\n");
	printf("\t-h\tthe help message\n");
}

/**
 *	Parse command line argument.	
 *
 * 	Return 0 if parse success, -1 on error.
 */
static int 
_parse_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":n:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'n':
			_g_ncpus = atoi(optarg);
			if (_g_ncpus < 1) {
				printf("the cpu number is error\n");
				return -1;
			}
			break;

		case 'h':
			return -1;

		case ':':
			printf("Option %c missing argument\n", optopt);
			return -1;

		case '?':
			printf("Unknowed option %c\n", optopt);
			return -1;
		}
	}

	if (argc != optind)
		return -1;

	return 0;
}

/**
 *	Init some global resource used in program.	
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_initiate(void)
{
	return 0;
}


/**
 *	Release global resource alloced by _initiate().	
 *
 * 	No Return.
 */
static void 
_release(void)
{
}

static int 
_get_sample(cpu_usage_t *usg, int ncpus)
{
	FILE *fp = NULL;
	char line[1024];
	int i;

	memset(usg, 0,  sizeof(cpu_usage_t) * ncpus);
	fp = fopen("/proc/stat", "r");
	if (fp == NULL)
		return -1;

	for (i = 0; i < ncpus; i++) {
		fgets(line, 1023, fp);
		sscanf(line, "%s %lld %lld %lld %lld", 
		       usg[i].name, &(usg[i].user), &(usg[i].nice), 
		       &(usg[i].kern), &(usg[i].idle));
		printf("name %s user %lld nice %lld kern %lld idle %lld\n", 
			usg[i].name, (usg[i].user), (usg[i].nice), 
		       (usg[i].kern), (usg[i].idle));
	}

	fclose(fp);

	return 0;
}


static void 
_print_usage(cpu_usage_t *usg1, cpu_usage_t *usg2, int ncpus)
{
	int i;
	long long user, nice, kern, idle, total;

	printf("name: user\tnice\tkern\tidle\tusage\n");
	for (i = 0; i < ncpus; i++) {
		user = usg2[i].user - usg1[i].user;
		nice = usg2[i].nice - usg1[i].nice;
		kern = usg2[i].kern - usg1[i].kern;
		idle = usg2[i].idle - usg1[i].idle;
		total = user + nice + kern + idle;
//		printf("user %lld nice %lld kern %lld idle %lld total %lld\n", 
//		       user, nice, kern, idle, total);

		printf("%s:\t%2.1f\t%2.1f\t%2.1f\t%2.1f\t%2.1f\n", 
		       usg1[i].name,
		       (100.0 * user / total), 
		       (100.0 * nice / total),
		       (100.0 * kern / total),
		       (100.0 * idle / total), 
		       (100.0 * (user + nice + kern) / total));
	}

	printf("\n\n");
}


/**
 *	The main entry of program.	
 *
 * 	Return 0 if success, other value on error.
 */
int 
main(int argc, char **argv)
{
	cpu_usage_t *usg1, *usg2;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	usg1 = malloc(sizeof(cpu_usage_t) * (_g_ncpus + 1));
	usg2 = malloc(sizeof(cpu_usage_t) * (_g_ncpus + 1));

	while (1) {
		_get_sample(usg1, _g_ncpus + 1);
		usleep(100000);
		_get_sample(usg2, _g_ncpus + 1);

		_print_usage(usg1, usg2, _g_ncpus + 1);
		sleep(1);

	}

	_release();

	return 0;
}



