/**
 *	@file	sysutil_test.c
 *
 *	@brief	test program for all sysutil APIs.
 *
 *	@author	Forrest.zhang
 *	
 *	@date	2012-07-23
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>

#include "cpu_util.h"
#include "mem_util.h"
#include "process_util.h"
#include "pidfile_util.h"

enum {
	CPU_TEST,
	MEM_TEST,
	PROCESS_TEST,
	PIDFILE_TEST,
};

static int		_g_type;
static int		_g_cmd;
static pid_t		_g_pid;
static char		_g_prog[32];
static u_int32_t	_g_cpu;

static void 
_cpu_usage(void)
{
	printf("\tcpu options:\n");
	printf("\t\t-n\t\tget cpu number\n");
	printf("\t\t-f\t\tget cpu frequence\n");
	printf("\t\t-t\t\tget total cpu usage\n");
	printf("\t\t-a\t\tget all cpus usage\n");
	printf("\t\t-h\t\tshow help message\n");
}

#define	CPU_GET_NUMBER		0x00000001
#define	CPU_GET_FREQUENCE	0x00000002
#define	CPU_GET_TOTAL_USAGE	0x00000004
#define CPU_GET_ALL_USAGE	0x00000008
#define	CPU_GET_ALL		0x0000ffff

static int 
_parse_cpu_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":nftah";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {

		case 'n':
			_g_cmd |= CPU_GET_NUMBER;
			break;
		case 'f':
			_g_cmd |= CPU_GET_FREQUENCE;
			break;
		case 't':
			_g_cmd |= CPU_GET_TOTAL_USAGE;
			break;
		case 'a':
			_g_cmd |= CPU_GET_ALL_USAGE;
			break;
		case 'h':
			_cpu_usage();
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

	if (_g_cmd == 0)
		_g_cmd = CPU_GET_ALL;

	return 0;
}

static void
_mem_usage(void)
{
	printf("\tmem options:\n");
	printf("\t\t-t\t\tget total memory size\n");
	printf("\t\t-f\t\tget freed memory size\n");
	printf("\t\t-u\t\tget used memory size\n");
	printf("\t\t-s\t\tget memory usage\n");
	printf("\t\t-h\t\tshow help message\n");
}

#define	MEM_GET_TOTAL		0x00000001
#define MEM_GET_FREED		0x00000002
#define MEM_GET_USED		0x00000004
#define MEM_GET_USAGE		0x00000008
#define	MEM_GET_ALL		0x0000ffff

static int 
_parse_mem_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":tfush";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {

		case 't':
			_g_cmd |= MEM_GET_TOTAL;
			break;
		case 'f':
			_g_cmd |= MEM_GET_FREED;
			break;
		case 'u':
			_g_cmd |= MEM_GET_USED;
			break;
		case 's':
			_g_cmd |= MEM_GET_USAGE;
			break;
		case 'h':
			_mem_usage();
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

	if (_g_cmd == 0)
		_g_cmd = MEM_GET_ALL;

	return 0;
}

static void 
_process_usage(void)
{
	printf("\tprocess options:\n");
	printf("\t\t-n <name>\tthe process name\n");
	printf("\t\t-p <pid>\tthe process ID\n");
	printf("\t\t-r\t\tget process RSS size\n");
	printf("\t\t-f\t\tfind process's pids\n");
	printf("\t\t-e\t\tcheck process exist or not\n");
	printf("\t\t-b <cpu>\tbind process to CPU\n");
	printf("\t\t-h\t\tshow help message\n");
}

#define	PROCESS_GET_RSS		0x00000001
#define PROCESS_GET_FIND	0x00000002
#define PROCESS_GET_EXIST	0x00000004
#define PROCESS_BIND_CPU	0x00000008
#define PROCESS_RENAME		0x00000010

static int 
_parse_process_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":n:p:ferb:h";
	int n;
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {

		case 'n':
			strncpy(_g_prog, optarg, sizeof(_g_prog) - 1);
			break;
		case 'p':
			_g_pid = atoi(optarg);
			if (_g_pid < 0) {
				printf("pid %s is invalid\n", optarg);
				return -1;
			}
			break;
		case 'f':
			_g_cmd |= PROCESS_GET_FIND;
			break;
		case 'e':
			_g_cmd |= PROCESS_GET_EXIST;
			break;
		case 'b':
			n = sscanf(optarg, "%x", &_g_cpu);
			if (n != 1) {
				printf("cpu %s is invalid\n", optarg);
				return -1;
			}
			_g_cmd |= PROCESS_BIND_CPU;
			break;
		case 'r':
			_g_cmd |= PROCESS_GET_RSS;
			break;
		case 'h':
			_process_usage();
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

	if (_g_cmd == 0) {
		_process_usage();
		return -1;
	}

	return 0;
}

static void 
_pidfile_usage(void)
{
	printf("\tpidfile options:\n");
	printf("\t\t-n <name>\tprocess name\n");
	printf("\t\t-p <pid>\tprocess ID\n");
	printf("\t\t-e\t\tcheck process <-n name> exist or not\n");
	printf("\t\t-r\t\tget pid from process <name>'d pidfile\n");
	printf("\t\t-a\t\tadd new pidfile for <name> <pid>\n");
	printf("\t\t-d\t\tdelete pidfile for program <name>\n");
	printf("\t\t-h\t\tshow help message\n");
}

#define	PIDFILE_GET_PID		0x00000001
#define PIDFILE_GET_EXIST	0x00000002
#define PIDFILE_ADD		0x00000004
#define PIDFILE_DEL		0x00000008

static int 
_parse_pidfile_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":n:p:eradh";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
		case 'n':
			strncpy(_g_prog, optarg, sizeof(_g_prog) - 1);
			break;
		case 'p':
			_g_pid = atoi(optarg);
			if (_g_pid < 0) {
				printf("pid %s is invalid\n", optarg);
				return -1;
			}
			break;
		case 'e':
			_g_cmd |= PIDFILE_GET_EXIST;
			break;
		case 'r':
			_g_cmd |= PIDFILE_GET_PID;
			break;

		case 'a':
			_g_cmd |= PIDFILE_ADD;
			break;
		case 'd':
			_g_cmd |= PIDFILE_DEL;
			break;
		case 'h':
			_pidfile_usage();
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

	if (_g_cmd == 0) {
		_process_usage();
		return -1;
	}

	return 0;
}

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("sysutil_test <cpu|mem|process|pidfile> <options>\n");
	printf("\n");
	printf("\t-h\tshow full help message\n");
	printf("\n");
	_cpu_usage();
	printf("\n");
	_mem_usage();
	printf("\n");
	_process_usage();
	printf("\n");
	_pidfile_usage();
}

/**
 *	Parse command line argument.	
 *
 * 	Return 0 if parse success, -1 on error.
 */
static int 
_parse_cmd(int argc, char **argv)
{
	if (argc < 2) {
		_usage();
		return -1;
	}

	if (strcmp(argv[1], "cpu") == 0)
		_g_type = CPU_TEST;
	else if (strcmp(argv[1], "mem") == 0)
		_g_type = MEM_TEST;
	else if (strcmp(argv[1], "process") == 0)
		_g_type = PROCESS_TEST;
	else if (strcmp(argv[1], "pidfile") == 0)
		_g_type = PIDFILE_TEST;
	else if (strcmp(argv[1], "-h") == 0) {
		_usage();
		exit (0);
	}
	else {
		_usage();
		return -1;
	}

	argc--;
	argv++;
	switch (_g_type) {
	case CPU_TEST:
		return _parse_cpu_cmd(argc, argv);
	case MEM_TEST:
		return _parse_mem_cmd(argc, argv);
	case PROCESS_TEST:
		return _parse_process_cmd(argc, argv);
	case PIDFILE_TEST:
		return _parse_pidfile_cmd(argc, argv);
	default:
		return -1;
	}

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
_cpu_test(void)
{
	int cpus[128] = {0};
	int ncpu;
	int i;

	if (_g_cmd & CPU_GET_NUMBER)
		printf("<cpu> number: %d\n", cpu_get_number());

	if (_g_cmd & CPU_GET_FREQUENCE)
		printf("<cpu> frequence: %d\n", cpu_get_frequence());

	if (_g_cmd & CPU_GET_TOTAL_USAGE) {
		ncpu = cpu_get_number();		
		printf("<cpu> total usage: %d%%\n", cpu_get_total_usage());
	}

	if (_g_cmd & CPU_GET_ALL_USAGE) {
		ncpu = cpu_get_number();		
		cpu_get_usage(cpus, ncpu);
		for (i = 0; i < ncpu; i++) {
			printf("\tcpu(%d) usage: %d%%\n", i, cpus[i]);
		}
	}

	return 0;
}

static int 
_mem_test(void)
{
	if (_g_cmd & MEM_GET_TOTAL)
		printf("<mem> total: %dMB\n", mem_get_total());

	if (_g_cmd & MEM_GET_FREED)
		printf("<mem> freed: %dMB\n", mem_get_freed());

	if (_g_cmd & MEM_GET_USED)
		printf("<mem> used: %dMB\n", mem_get_used());

	if (_g_cmd & MEM_GET_USAGE)
		printf("<mem> usage: %d%%\n", mem_get_usage());

	return 0;
}


static int  
_process_test(char **argv)
{
	char *ptr, *ptr1, *ptr2;
	pid_t *pids, pid;
	int i;

	if (!_g_pid) {
		_g_pid = getpid();
		printf("<process> self pid: %d\n", getpid());
	}

	if (_g_cmd & PROCESS_GET_FIND) {
		if (strlen(_g_prog) < 1) {
			printf("<process> need new name by -n option\n");
			return -1;
		}
		pids = process_find(_g_prog);
		if (!pids) {
			printf("<process> find %s not running\n", _g_prog);
		}
		else {
			printf("<process> find %s pids:\n", _g_prog);
			i = 0;
			pid = pids[i++];
			while (pid > 0) {
				printf("\tpid: %d\n", pid);
				pid = pids[i];
			}
			free(pids);
		}
	}
	
	if (_g_cmd & PROCESS_GET_EXIST) {
		if (process_is_exist(_g_pid))
			printf("<process> pid %d existed\n", _g_pid);
		else 
			printf("<process> pid %d not existed\n", _g_pid);
	}

	if (_g_cmd & PROCESS_GET_RSS) {
		printf("<process> rss: %luKB\n", process_get_rss());
		ptr = malloc(40960);
		memset(ptr, 0, 40960);
	
		sleep(10);
		printf("<process> rss: %luKB\n", process_get_rss());

		ptr1 = malloc(4096);
		memset(ptr1, 0, 4096);
		ptr2 = malloc(4096);
		memset(ptr2, 0, 4096);

		sleep(10);
		printf("<process> rss: %luKB\n", process_get_rss());

		free(ptr);
		free(ptr1);
		free(ptr2);
		sleep(10);
		printf("<process> rss: %luKB\n", process_get_rss());
	}

	/* rename self to @_g_prog */
	if (_g_cmd & PROCESS_RENAME) {
		if (strlen(_g_prog) < 1) {
			printf("need provide new name by -n option\n");
			return -1;
		}
		if (process_rename(_g_prog, argv))
			printf("<process> rename %s failed\n", _g_prog);
		else 
			printf("<process> rename %s success\n", _g_prog);
		sleep(10);
	}

	return 0;
}

static int 
_pidfile_test(void)
{
	pid_t pid;

	if (!_g_pid) {
		_g_pid = getpid();
		printf("<pidfile> self pid: %d\n", getpid());
	}

	if (_g_cmd & PIDFILE_GET_EXIST) {
		if (strlen(_g_prog) < 1) {
			printf("<pidfile> need program name by -n option\n");
			return -1;
		}
		if (pidfile_is_exist(_g_prog))
			printf("<pidfile> %s is existed\n", _g_prog);
		else
			printf("<pidfile> %s not existed\n", _g_prog);
	}

	if (_g_cmd & PIDFILE_GET_PID) {
		if (strlen(_g_prog) < 1) {
			printf("<pidfile> need program name by -n option\n");
			return -1;
		}
		pid = pidfile_get_pid(_g_prog);
		if (pid < 0)
			printf("<pidfile> program %s not existed\n", 
			       _g_prog);
		else
			printf("<pidfile> program %s pid: %d\n", 
			       _g_prog, pid);
	}

	if (_g_cmd & PIDFILE_ADD) {
		if (strlen(_g_prog) < 1) {
			printf("<pidfile> need program name by -n option\n");
			return -1;
		}
		if (pidfile_new(_g_prog, _g_pid)) 
			printf("<pidfile> add %s(%d) failed\n", 
			       _g_prog, _g_pid);
		else
			printf("<pidfile> add %s(%d) success\n",
			       _g_prog, _g_pid);
	}

	if (_g_cmd & PIDFILE_DEL) {
		if (strlen(_g_prog) < 1) {
			printf("<pidfile> need program name by -n option\n");
			return -1;
		}
		if (pidfile_del(_g_prog)) 
			printf("<pidfile> delete %s failed\n", _g_prog);
		else
			printf("<pidfile> delete %s success\n", _g_prog);
	}

	return 0;
}

/**
 *	The main entry of program.	
 *
 * 	Return 0 if success, other value on error.
 */
int 
main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	switch (_g_type) {
		
	case CPU_TEST:
		_cpu_test();
		break;

	case MEM_TEST:
		_mem_test();
		break;

	case PROCESS_TEST:
		_process_test(argv);
		break;

	case PIDFILE_TEST:
		_pidfile_test();
		break;

	default:
		break;
	}

	_release();

	return 0;
}



