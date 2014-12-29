/**
 *	@file	netopt_test.c
 *
 *	@brief	netopt test program
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
#include <net/if.h>

#include "cpu_util.h"
#include "net_procfs.h"
#include "net_sysfs.h"

static int	_g_type;
static int	_g_cmd;
static char	_g_ifname[IFNAMSIZ];

enum {
	TEST_SYSFS,
	TEST_CORE,
	TEST_IP4,
	TEST_IP6,
	TEST_TCP,
	TEST_IRQ,
};

#define	SYSFS_GET_VENDOR_ID		0x00000001
#define	SYSFS_GET_DEVICE_ID		0x00000002
#define	SYSFS_GET_INDEX			0x00000004
#define	SYSFS_GET_MTU			0x00000008
#define	SYSFS_GET_SPEED			0x00000010
#define	SYSFS_GET_HWADDR		0x00000020
#define	SYSFS_GET_CARRIER		0x00000040
#define	SYSFS_GET_IRQ			0x00000080
#define	SYSFS_GET_RPS_CPU		0x00000100
#define	SYSFS_SET_RPS_CPU		0x01000000
#define	SYSFS_GET_RPS_FLOW		0x00000200
#define	SYSFS_SET_RPS_FLOW		0x02000000
#define	SYSFS_GET_ALL			0x00ffffff

static u_int64_t	_g_sysfs_rps_cpu;
static int		_g_sysfs_rps_flow;

static void 
_sysfs_usage(void)
{
	printf("\tsysfs options:\n");
	printf("\t\t-n <name>\tthe interface name\n");
	printf("\t\t-v\t\tget the vendor ID\n");
	printf("\t\t-d\t\tget the device ID\n");
	printf("\t\t-i\t\tget the index\n");
	printf("\t\t-m\t\tget the MTU\n");
	printf("\t\t-s\t\tget the speed\n");
	printf("\t\t-a\t\tget the HW address\n");
	printf("\t\t-l\t\tget the carrier\n");
	printf("\t\t-q\t\tget all IRQ number\n");
	printf("\t\t-c\t\tget the RPS cpu flags\n");
	printf("\t\t-C <cpu>\tset the RPS cpu flags\n");
	printf("\t\t-f\t\tget the RPS flow count\n");
	printf("\t\t-F <N>\t\tset the RPS flow count\n");
	printf("\t\t-h\t\tshow sysfs help message\n");
}

static int 
_parse_sysfs_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":n:vdimsalqcC:fF:h";
	int n;

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {

		case 'n':
			strncpy(_g_ifname, optarg, IFNAMSIZ - 1);
			break;
		case 'v':
			_g_cmd |= SYSFS_GET_VENDOR_ID;
			break;
		case 'd':
			_g_cmd |= SYSFS_GET_DEVICE_ID;
			break;
		case 'i':
			_g_cmd |= SYSFS_GET_INDEX;
			break;
		case 'm':
			_g_cmd |= SYSFS_GET_MTU;
			break;
		case 's':
			_g_cmd |= SYSFS_GET_SPEED;
			break;
		case 'a':
			_g_cmd |= SYSFS_GET_HWADDR;
			break;
		case 'l':
			_g_cmd |= SYSFS_GET_CARRIER;
			break;
		case 'q':
			_g_cmd |= SYSFS_GET_IRQ;
			break;
		case 'c':
			_g_cmd |= SYSFS_GET_RPS_CPU;
			break;
		case 'C':
			n = sscanf(optarg, "%lx", &_g_sysfs_rps_cpu);
			if (n != 1) {
				printf("rps_cpu %s is invalid\n", optarg);
				return -1;
			}
			_g_cmd |= SYSFS_SET_RPS_CPU;
			break;
		case 'f':
			_g_cmd |= SYSFS_GET_RPS_FLOW;
			break;
		case 'F':
			n = sscanf(optarg, "%d", &_g_sysfs_rps_flow);
			if (n != 1) {
				printf("rps_flow %s is invalid\n", optarg);
				return -1;
			}
			_g_cmd |= SYSFS_SET_RPS_FLOW;
			break;
		case 'h':
			_sysfs_usage();
			return -1;
		case ':':
			printf("Option %c missing argument\n", optopt);
			return -1;
		case '?':
			printf("Unknowed option %c\n", optopt);
			return -1;
		}
	}

	if (strlen(_g_ifname) < 1) {
		printf("need set interface name\n");
		return -1;
	}

	if (nets_is_exist(_g_ifname) == 0) {
		printf("interface %s is not exist\n", _g_ifname);
		return -1;
	}

	if (argc != optind)
		return -1;

	if (_g_cmd == 0)
		_g_cmd = SYSFS_GET_ALL;

	return 0;
}

#define	CORE_GET_RMEM_DEFAULT		0x00000001
#define	CORE_SET_RMEM_DEFAULT		0x00010000
#define CORE_GET_RMEM_MAX		0x00000002
#define CORE_SET_RMEM_MAX		0x00020000
#define	CORE_GET_WMEM_DEFAULT		0x00000004
#define	CORE_SET_WMEM_DEFAULT		0x00040000
#define	CORE_GET_WMEM_MAX		0x00000008
#define	CORE_SET_WMEM_MAX		0x00080000
#define CORE_GET_SOMAXCONN		0x00000010
#define CORE_SET_SOMAXCONN		0x00100000
#define CORE_GET_RPS			0x00000020
#define CORE_SET_RPS			0x00200000
#define	CORE_GET_ALL			0x0000ffff

static int	_g_core_rmem_default;
static int	_g_core_rmem_max;
static int	_g_core_wmem_default;
static int	_g_core_wmem_max;
static int	_g_core_somaxconn;
static int	_g_core_rps;

static void 
_core_usage(void)
{
	printf("\tcore options:\n");
	printf("\t\t-r\t\tget the rmem default\n");
	printf("\t\t-R <N>\t\tset the rmem default\n");
	printf("\t\t-m\t\tget the rmem max\n");
	printf("\t\t-M <N>\t\tset the rmem max\n");
	printf("\t\t-d\t\tget the wmem default\n");
	printf("\t\t-D <N>\t\tset the wmen default\n");
	printf("\t\t-w\t\tget the wmem max\n");
	printf("\t\t-W <N>\t\tset the wmem max\n");
	printf("\t\t-s\t\tget the somaxconn\n");
	printf("\t\t-S <N>\t\tset the somamxconn\n");
	printf("\t\t-f\t\tget the RFS flow table entry\n");
	printf("\t\t-F <N>\t\tset the RFS flow table entry\n");
	printf("\t\t-h\t\tshow core help message\n");
}

static int 
_parse_core_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":rR:mM:dD:wW:sS:fF:h";

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {

		case 'r':
			_g_cmd |= CORE_GET_RMEM_DEFAULT;
			break;
		case 'R':
			_g_core_rmem_default = atoi(optarg);
			if (_g_core_rmem_default < 0) {
				printf("rmem_default %s is invalid\n", optarg);
				return -1;
			}
			_g_cmd |= CORE_SET_RMEM_DEFAULT;
			break;
		case 'm':
			_g_cmd |= CORE_GET_RMEM_MAX;
			break;
		case 'M':
			_g_core_rmem_max = atoi(optarg);
			if (_g_core_rmem_max < 0) {
				printf("rmem_max %s is invalid\n", optarg);
				return -1;
			}
			_g_cmd |= CORE_SET_RMEM_MAX;
			break;
		case 'd':
			_g_cmd |= CORE_GET_WMEM_DEFAULT;
			break;
		case 'D':
			_g_core_wmem_default = atoi(optarg);
			if (_g_core_wmem_default < 0) {
				printf("wmem_default %s is invalid\n", optarg);
				return -1;
			}
			_g_cmd |= CORE_SET_WMEM_DEFAULT;
			break;
		case 'w':
			_g_cmd |= CORE_GET_WMEM_MAX;
			break;
		case 'W':
			_g_core_wmem_max = atoi(optarg);
			if (_g_core_wmem_max < 0) {
				printf("wmem_max %s is invalid\n", optarg);
				return -1;
			}
			_g_cmd |= CORE_SET_WMEM_MAX;
			break;
		case 's':
			_g_cmd |= CORE_GET_SOMAXCONN;
			break;
		case 'S':
			_g_core_somaxconn = atoi(optarg);
			if (_g_core_somaxconn < 0) {
				printf("somaxconn %s is invalid\n", optarg);
				return -1;
			}
			_g_cmd |= CORE_SET_SOMAXCONN;
			break;
		case 'f':
			_g_cmd |= CORE_GET_RPS;
			break;
		case 'F':
			_g_core_rps = atoi(optarg);
			if (_g_core_rps < 0) {
				printf("rps %s is invalid\n", optarg);
				return -1;
			}
			_g_cmd |= CORE_SET_RPS;
			break;
		case 'h':
			_core_usage();
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
		_g_cmd = CORE_GET_ALL;

	return 0;
}

#define	IP4_GET_FORWARD			0x00000001
#define	IP4_SET_FORWARD			0x00010000
#define	IP4_GET_LOCAL_PORT_RANGE	0x00000002
#define	IP4_SET_LOCAL_PORT_RANGE	0x00020000
#define IP4_GET_PROMOTE_SECONDARIES	0x00000004
#define	IP4_SET_PROMOTE_SECONDARIES	0x00040000
#define	IP4_GET_ALL			0x0000ffff

static int	_g_ip4_forward;
static char	_g_ip4_local_port_range[32];
static int	_g_ip4_promote_secondaries;

static void 
_ip4_usage(void)
{
	printf("\tip4 options:\n");
	printf("\t\t-n <name>\tinterface name\n");
	printf("\t\t-f\t\tget the IP-Forward\n");
	printf("\t\t-F <0|1>\tset the IP-Forware value\n");
	printf("\t\t-l\t\tget the local-port-range\n");
	printf("\t\t-L <N-N>\tset the local-port-range\n");
	printf("\t\t-p\t\tget promote-secondaries\n");
	printf("\t\t-P <0|1>\tSet the promote-secondaries\n");
	printf("\t\t-h\t\tshow ip4 help message\n");
}

static int 
_parse_ip4_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":n:fF:lL:pP:h";

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {

		case 'n':
			strncpy(_g_ifname, optarg, IFNAMSIZ - 1);
			break;
		case 'f':
			_g_cmd |= IP4_GET_FORWARD;
			break;
		case 'F':
			_g_ip4_forward = atoi(optarg);
			if (_g_ip4_forward < 0) {
				printf("forwarding %s is invalid\n", optarg);
				return -1;
			}
			_g_cmd |= IP4_SET_FORWARD;
			break;
		case 'l':
			_g_cmd |= IP4_GET_LOCAL_PORT_RANGE;
			break;
		case 'L':
			strncpy(_g_ip4_local_port_range, optarg, 
				sizeof(_g_ip4_local_port_range) - 1);
			_g_cmd |= IP4_SET_LOCAL_PORT_RANGE;
			break;
		case 'p':
			_g_cmd |= IP4_GET_PROMOTE_SECONDARIES;
			break;
		case 'P':
			_g_ip4_promote_secondaries = atoi(optarg);
			if (_g_ip4_promote_secondaries < 0) {
				printf("promote_secondaries %s is invalid\n", 
				       optarg);
				return -1;
			}
			_g_cmd |= IP4_SET_PROMOTE_SECONDARIES;
			break;
		case 'h':
			_ip4_usage();
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
		_g_cmd = IP4_GET_ALL;

	return 0;
}

#define	IP6_GET_ACCEPT_DAD		0x00000001
#define	IP6_SET_ACCEPT_DAD		0x00010000
#define	IP6_GET_FORWARDING		0x00000002
#define IP6_SET_FORWARDING		0x00020000
#define IP6_GET_MTU			0x00000004
#define	IP6_SET_MTU			0x00040000
#define IP6_GET_DISABLE			0x00000008
#define IP6_SET_DISABLE			0x00080000
#define IP6_GET_ALL			0x0000ffff

static int	_g_ip6_accept_dad;
static int	_g_ip6_forwarding;
static int	_g_ip6_mtu;
static int	_g_ip6_disable;

static void 
_ip6_usage(void)
{
	printf("\tip6 options:\n");
	printf("\t\t-n <name>\tthe interface name or <default|all>\n");
	printf("\t\t-a\t\tget the accept-dad value\n");
	printf("\t\t-A <0|1>\tset the accept-dad value\n");
	printf("\t\t-f\t\tget the forwarding value\n");
	printf("\t\t-F <0|1>\tset the forwarding value\n");
	printf("\t\t-m\t\tget the mtu value\n");
	printf("\t\t-M <N>\t\tset the mtu value\n");
	printf("\t\t-d\t\tget the disable value\n");
	printf("\t\t-D <0|1>\tset the disable value\n");
	printf("\t\t-h\t\tshow ip4 help message\n");
}

static int 
_parse_ip6_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":n:aA:fF:mM:dD:h";

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'n':
			strncpy(_g_ifname, optarg, IFNAMSIZ - 1);
			break;			
		case 'a':
			_g_cmd |= IP6_GET_ACCEPT_DAD;
			break;
		case 'A':
			_g_ip6_accept_dad = atoi(optarg);
			if (_g_ip6_accept_dad < 0) {
				printf("accept_dad %s is invalid\n", optarg);
				return -1;
			}
			_g_cmd |= IP6_SET_ACCEPT_DAD;
			break;
		case 'f':
			_g_cmd |= IP6_GET_FORWARDING;
			break;
		case 'F':
			_g_ip6_forwarding = atoi(optarg);
			if (_g_ip6_forwarding < 0) {
				printf("forwarding %s is invalid\n", optarg);
				return -1;
			}
			_g_cmd |= IP6_SET_FORWARDING;
			break;
		case 'm':
			_g_cmd |= IP6_GET_MTU;
			break;
		case 'M':
			_g_ip6_mtu = atoi(optarg);
			if (_g_ip6_mtu < 0) {
				printf("mtu %s is invalid\n", optarg);
				return -1;
			}
			_g_cmd |= IP6_SET_MTU;
			break;
		case 'd':
			_g_cmd |= IP6_GET_DISABLE;
			break;
		case 'D':
			_g_ip6_disable = atoi(optarg);
			if (_g_ip6_disable < 0) {
				printf("disable %s is invalid\n", optarg);
				return -1;
			}
			_g_cmd |= IP6_SET_DISABLE;
			break;
		case 'h':
			_ip6_usage();
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

	if (strlen(_g_ifname) < 1) {
		printf("need set interface name\n");
		return -1;
	}

	if (nets_is_exist(_g_ifname) == 0 && 
	    strcmp(_g_ifname, "all") && 
	    strcmp(_g_ifname, "default")) 
	{
		printf("interface %s is not exist\n", _g_ifname);
		return -1;
	}

	if (_g_cmd == 0)
		_g_cmd = IP6_GET_ALL;

	return 0;
}

#define	TCP_GET_MEM			0x00000001
#define TCP_SET_MEM			0x00010000
#define TCP_GET_RMEM			0x00000002
#define TCP_SET_RMEM			0x00020000
#define TCP_GET_WMEM			0x00000004
#define TCP_SET_WMEM			0x00040000
#define TCP_GET_TIMESTAMPS		0x00000008
#define TCP_SET_TIMESTAMPS		0x00080000
#define TCP_GET_TW_REUSE		0x00000010
#define TCP_SET_TW_REUSE		0x00100000
#define TCP_GET_TW_RECYCLE		0x00000020
#define TCP_SET_TW_RECYCLE		0x00200000
#define TCP_GET_FIN_TIMEOUT		0x00000040
#define TCP_SET_FIN_TIMEOUT		0x00400000
#define TCP_GET_MAX_SYN_BACKLOG		0x00000080
#define TCP_SET_MAX_SYN_BACKLOG		0x00800000
#define TCP_GET_MAX_TW_BUCKETS		0x00000100
#define TCP_SET_MAX_TW_BUCKETS		0x01000000
#define	TCP_GET_ALL			0x0000ffff

static char	_g_tcp_mem[32];
static char	_g_tcp_rmem[32];
static char	_g_tcp_wmem[32];
static int	_g_tcp_timestamps;
static int	_g_tcp_tw_reuse;
static int	_g_tcp_tw_recycle;
static int	_g_tcp_fin_timeout;
static int	_g_tcp_max_syn_backlog;
static int	_g_tcp_max_tw_buckets;

static void 
_tcp_usage(void)
{
	printf("\tnetopt_test tcp4 options:\n");
	printf("\t\t-m\t\tget tcp mem\n");
	printf("\t\t-M <N>\t\tset tcp mem\n");
	printf("\t\t-r\t\tget tcp rmem\n");
	printf("\t\t-R <N>\t\tset tcp rmem\n");
	printf("\t\t-w\t\tget tcp wmem\n");
	printf("\t\t-W <N>\t\tset tcp wmem\n");
	printf("\t\t-t\t\tget tcp timestamp\n");
	printf("\t\t-T <N>\t\tset tcp timestamp\n");
	printf("\t\t-u\t\tget tcp tw_reuse\n");
	printf("\t\t-U <N>\t\tset tcp tw_reuse\n");
	printf("\t\t-c\t\tget tcp tw_recycle\n");
	printf("\t\t-C <N>\t\tset tcp tw_recycle\n");
	printf("\t\t-b\t\tget tcp max tw buckets\n");
	printf("\t\t-B <N>\t\tset tcp max buckets\n");
	printf("\t\t-f\t\tget tcp fin timeout\n");
	printf("\t\t-F <N>\t\tset tcp fin timeout\n");
	printf("\t\t-s\t\tget tcp max syn backlog\n");
	printf("\t\t-S <N>\t\tset tcp max syn backlog\n");
	printf("\t\t-h\t\tshow help message\n");
}


static int 
_parse_tcp_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":mM:rR:wW:tT:uU:cC:bB:fF:sS:h";

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) {

		case 'm':
			_g_cmd |= TCP_GET_MEM;
			break;
		case 'M':
			strncpy(_g_tcp_mem, optarg, sizeof(_g_tcp_mem) - 1);
			_g_cmd |= TCP_SET_MEM;
			break;
		case 'r':
			_g_cmd |= TCP_GET_RMEM;
			break;
		case 'R':
			strncpy(_g_tcp_rmem, optarg, sizeof(_g_tcp_rmem) - 1);
			_g_cmd |= TCP_SET_RMEM;
			break;
		case 'w':
			_g_cmd |= TCP_GET_WMEM;
			break;
		case 'W':
			strncpy(_g_tcp_wmem, optarg, sizeof(_g_tcp_wmem) - 1);
			_g_cmd |= TCP_SET_RMEM;
			break;
		case 't':
			_g_cmd |= TCP_GET_TIMESTAMPS;
			break;
		case 'T':
			_g_tcp_timestamps = atoi(optarg);
			if (_g_tcp_timestamps < 0) {
				printf("tcp timestamps %s is invalid\n", 
				       optarg);
				return -1;
			}
			_g_cmd |= TCP_SET_TIMESTAMPS;
			break;
		case 'u':
			_g_cmd |= TCP_GET_TW_REUSE;
			break;
		case 'U':
			_g_tcp_tw_reuse = atoi(optarg);
			if (_g_tcp_tw_reuse < 0) {
				printf("tcp_tw_reuse %s is invalid\n", 
				       optarg);
				return -1;
			}
			_g_cmd |= TCP_SET_TW_REUSE;
			break;
		case 'c':
			_g_cmd |= TCP_GET_TW_RECYCLE;
			break;
		case 'C':
			_g_tcp_tw_recycle = atoi(optarg);
			if (_g_tcp_tw_recycle < 0) {
				printf("tcp tw_recycle %s is invalid\n", 
				       optarg);
				return -1;
			}
			_g_cmd |= TCP_SET_TW_RECYCLE;
			break;
		case 'b':
			_g_cmd |= TCP_GET_MAX_TW_BUCKETS;
			break;
		case 'B':
			_g_tcp_max_tw_buckets = atoi(optarg);
			if (_g_tcp_max_tw_buckets < 0) {
				printf("tcp max_tw_buckets %s is invalid\n", 
				       optarg);
				return -1;
			}
			_g_cmd |= TCP_SET_MAX_TW_BUCKETS;
			break;
		case 'f':
			_g_cmd |= TCP_GET_FIN_TIMEOUT;
			break;
		case 'F':
			_g_tcp_fin_timeout = atoi(optarg);
			if (_g_tcp_fin_timeout < 0) {
				printf("tcp fin_timeout %s is invalid\n", 
				       optarg);
				return -1;
			}
			_g_cmd |= TCP_SET_FIN_TIMEOUT;
			break;
		case 's':
			_g_cmd |= TCP_GET_MAX_SYN_BACKLOG;
			break;
		case 'S':
			_g_tcp_max_syn_backlog = atoi(optarg);
			if (_g_tcp_max_syn_backlog < 0) {
				printf("tcp max_syn_backlog %s is invalid\n", 
				       optarg);
				return -1;
			}
			_g_cmd |= TCP_SET_MAX_SYN_BACKLOG;
			break;
		case 'h':
			_tcp_usage();
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
		_g_cmd = TCP_GET_ALL;

	return 0;
}

static void 
_irq_usage(void) 
{
	printf("\tirq <options>\n");
	printf("\t\t-n <name>\tport name\n");
	printf("\t\t-c\t\tshow interface IRQ cpu flag\n");
	printf("\t\t-C <algo>\tset interface IRQ bind cpu algorithm:\n");
	printf("\t\t\t\trr\n");
	printf("\t\t\t\t  Round-Robin algorith\n");
	printf("\t\t\t\t  set nic1 irqs to cpu0-N\n");
	printf("\t\t\t\t  then set nic2 to cpuN-M, ...\n");
	printf("\t\t\t\todd\n");
	printf("\t\t\t\t  set all nics irqs to odd cpus\n");
	printf("\t\t\t\teven:\n");
	printf("\t\t\t\t  set all nics irqs to even cpus\n");
	printf("\t\t\t\tfull\n");
	printf("\t\t\t\t  set nicN all irqs to all cpus,\n");
	printf("\t\t\t\t  each IRQ have N-cpu/N-irq cpus\n");
	printf("\t\t-R <range>\tset interface IRQ bind cpu range:\n");
	printf("\t\t\t\tfull\n");
	printf("\t\t\t\t  Bind to all cpu range\n");
	printf("\t\t\t\tlow\n");
	printf("\t\t\t\t  Bind IRQ to lower CPUs, 0 to (maxCPUs/2 - 1)\n");
	printf("\t\t\t\thigh\n");
	printf("\t\t\t\t  Bind IRQ to higher CPUs, maxCPUs/2 to (maxCPUs - 1)\n");
	printf("\t\t-h\t\tshow help message\n");
}


#define	IRQ_GET_CPU			0x00000001
#define	IRQ_SET_CPU			0x00010000
#define IRQ_GET_ALL			0x0000ffff

#define	IRQ_MAX_INTF			20
#define IRQ_MAX_IRQ			32
enum {
	IRQ_BIND_RR,
	IRQ_BIND_ODD,
	IRQ_BIND_EVEN,
	IRQ_BIND_FULL,
};

enum {
	IRQ_HT_FULL,
	IRQ_HT_LOW,
	IRQ_HT_HIGH,
};

typedef struct intf_irq {
	char		name[IFNAMSIZ];
	int		irqs[IRQ_MAX_IRQ];
	u_int32_t	masks[IRQ_MAX_IRQ];
	int		nirq;
} intf_irq_t;

static intf_irq_t	_g_irq_intfs[IRQ_MAX_INTF];
static int		_g_irq_nintf;
static int 		_g_irq_max;
static int		_g_irq_algo;
static int		_g_irq_ht;

static int 
_parse_irq_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":n:cC:R:h";

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) {
		case 'n':
			if (_g_irq_nintf >= IRQ_MAX_INTF) {
				printf("too many interfaces %d\n", 
					_g_irq_nintf);
				return -1;
			}
			if (nets_is_exist(optarg) != 1) {
				printf("interface %s not exist\n",
					optarg);
				return -1;
			}
			strncpy(_g_irq_intfs[_g_irq_nintf].name, optarg, 
				IFNAMSIZ);
			_g_irq_nintf++;
			break;
		case 'c':
			_g_cmd |= IRQ_GET_CPU;
			break;
		case 'C':
			if (strcmp("rr", optarg) == 0)
				_g_irq_algo = IRQ_BIND_RR;
			else if (strcmp("odd", optarg) == 0)
				_g_irq_algo = IRQ_BIND_ODD;
			else if (strcmp("even", optarg) == 0)
				_g_irq_algo = IRQ_BIND_EVEN;
			else if (strcmp("full", optarg) == 0)
				_g_irq_algo = IRQ_BIND_FULL;
			else {
				printf("invalid CPU bind algorithm %s\n", 
					optarg);
				return -1;
			}
			_g_cmd |= IRQ_SET_CPU;
			break;
		case 'R':
			if (strcmp("full", optarg) == 0)
				_g_irq_ht = IRQ_HT_FULL;
			else if (strcmp("low", optarg) == 0)
				_g_irq_ht = IRQ_HT_LOW;
			else if (strcmp("high", optarg) == 0)
				_g_irq_ht = IRQ_HT_HIGH;
			else {
				printf("invalid cpu bind HT range %s\n", 
					optarg);
				return -1;
			}
			_g_cmd |= IRQ_SET_CPU;
			break;

		case 'h':
			_irq_usage();
			return -1;
		case ':':
			printf("Option %c missing argument\n", optopt);
			return -1;
		case '?':
			printf("Unknowed option %c\n", optopt);
			return -1;
		}
	}

	if (argc != optind) {
		_irq_usage();
		return -1;
	}

	if (_g_irq_nintf < 1) {
		printf("need using -n set interface\n");
		return -1;
	}

	if (_g_cmd == 0)
		_g_cmd = IRQ_GET_ALL;

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
	printf("netopt_test <sysfs|core|ip4|ip6|tcp> <options>\n");
	printf("\n");
	printf("\t-h\tshow help message\n");
	printf("\n");
	_sysfs_usage();
	printf("\n");
	_core_usage();
	printf("\n");
	_ip4_usage();
	printf("\n");
	_ip6_usage();
	printf("\n");
	_tcp_usage();
	printf("\n");
	_irq_usage();
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

	if (strcmp(argv[1], "sysfs") == 0)
		_g_type = TEST_SYSFS;
	else if (strcmp(argv[1], "core") == 0)
		_g_type = TEST_CORE;
	else if (strcmp(argv[1], "ip4") == 0)
		_g_type = TEST_IP4;
	else if (strcmp(argv[1], "ip6") == 0)
		_g_type = TEST_IP6;
	else if (strcmp(argv[1], "tcp") == 0)
		_g_type = TEST_TCP;
	else if (strcmp(argv[1], "irq") == 0)
		_g_type = TEST_IRQ;
	else if (strcmp(argv[1], "-h") == 0) {
		_usage();
		exit(0);
	}
	else {
		printf("invalid test type %s\n\n", argv[1]);
                _usage();
                return -1;
	}

	argc--;
	argv++;
	switch (_g_type) {

	case TEST_SYSFS:
		return _parse_sysfs_cmd(argc, argv);
	case TEST_CORE:
		return _parse_core_cmd(argc, argv);
	case TEST_IP4:
		return _parse_ip4_cmd(argc, argv);
	case TEST_IP6:
		return _parse_ip6_cmd(argc, argv);
	case TEST_TCP:
		return _parse_tcp_cmd(argc, argv);
	case TEST_IRQ:
		return _parse_irq_cmd(argc, argv);
	default:
		return -1;
	}
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
_sysfs_test(void)
{
	char hwaddr[32];
	int nirq;
	int irqs[64];
	int nmask;
	u_int64_t masks[64];
	int nflow;
	int flows[64];
	int i;
	int size;

	if (_g_cmd & SYSFS_GET_VENDOR_ID)
		printf("<sysfs> %s vendor id: 0x%x\n", _g_ifname,  
		       nets_get_vendor_id(_g_ifname));

	if (_g_cmd & SYSFS_GET_DEVICE_ID)
		printf("<sysfs> %s device id: 0x%x\n", _g_ifname, 
		       nets_get_device_id(_g_ifname));
	
	if (_g_cmd & SYSFS_GET_INDEX)
		printf("<sysfs> %s index: %d\n", _g_ifname, 
		       nets_get_index(_g_ifname));
	
	if (_g_cmd & SYSFS_GET_MTU)
		printf("<sysfs> %s mtu: %d\n", _g_ifname, 
		       nets_get_mtu(_g_ifname));

	if (_g_cmd & SYSFS_GET_SPEED)
		printf("<sysfs> %s speed: %d\n", _g_ifname, 
		       nets_get_speed(_g_ifname));

	if (_g_cmd & SYSFS_GET_CARRIER)
		printf("<sysfs> %s carrier: %d\n", _g_ifname, 
		       nets_get_carrier(_g_ifname));

	if (_g_cmd & SYSFS_GET_HWADDR)
		printf("<sysfs> %s hwaddr: %s\n", _g_ifname, 
		       nets_get_hwaddr(_g_ifname, hwaddr, sizeof(hwaddr)));

	if (_g_cmd & SYSFS_GET_IRQ) {
		size = sizeof(irqs)/sizeof(irqs[0]);
		nirq = nets_get_irqs(_g_ifname, irqs, size);
		if (nirq < 0) {
			printf("<sysfs> %s get irqs failed: %s\n",
				_g_ifname, nets_get_error());
		}
		else {
			printf("<sysfs> %s irq(%d): ", _g_ifname, nirq);
			for (i = 0; i < nirq; i++)
				printf("%d ", irqs[i]);
			printf("\n");
		}
	}

	if (_g_cmd & SYSFS_GET_RPS_CPU) {
		size = sizeof(masks)/sizeof(masks[0]);
		nmask = nets_get_rps_cpu(_g_ifname, masks, size);
		printf("<sysfs> %s rps cpu(%d): ", _g_ifname, nmask);
		for (i = 0; i < nmask; i++) 
			printf("0x%lx ", masks[i]);
		printf("\n");
	}

	if (_g_cmd & SYSFS_SET_RPS_CPU) {
		nmask = nets_get_rps_count(_g_ifname);
		if (nmask < 1) {
			printf("<sysfs> %s get rps cpu %d is error\n", 
			       _g_ifname, nmask);
		}
		memset(masks, 0, sizeof(masks));
		for (i = 0; i < nmask; i++) {
			masks[i] = _g_sysfs_rps_cpu;
		}
		if (nets_set_rps_cpu(_g_ifname, masks, nmask)) {
			printf("<sysfs> %s set rps cpu %lx failed\n", 
			       _g_ifname, _g_sysfs_rps_cpu);
			return -1;
		} 
		else {
			printf("<sysfs> %s set rps cpu %lx success\n", 
			       _g_ifname, _g_sysfs_rps_cpu);
		}
	}

	if (_g_cmd & SYSFS_GET_RPS_FLOW) {
		size = sizeof(flows)/sizeof(flows[0]);
		nflow = nets_get_rps_flow(_g_ifname, flows, size);
		printf("<sysfs> %s rps flow(%d): ", _g_ifname, nflow);
		for (i = 0; i < nflow; i++) 
			printf("%d ", flows[i]);
		printf("\n");
	}

	if (_g_cmd & SYSFS_SET_RPS_FLOW) {
		nflow = nets_get_rps_count(_g_ifname);
		if (nflow < 1) {
			printf("<sysfs> %s get rps cpu %d is error\n", 
			       _g_ifname, nmask);
		}
		memset(flows, 0, sizeof(flows));
		for (i = 0; i < nflow; i++) {
			flows[i] = _g_sysfs_rps_flow;
		}
		if (nets_set_rps_flow(_g_ifname, flows, nflow)) {
			printf("<sysfs> %s set rps flow %d failed\n", 
			       _g_ifname, _g_sysfs_rps_flow);
			return -1;
		} 
		else {
			printf("<sysfs> %s set rps flow %d success\n", 
			       _g_ifname, _g_sysfs_rps_flow);
		}
	}

	return 0;
}

static int 
_core_test(void)
{
	if (_g_cmd & CORE_GET_RMEM_DEFAULT)
		printf("<core> rmem default: %d\n", 
		       netp_core_get_rmem_default());

	if (_g_cmd & CORE_SET_RMEM_DEFAULT) {
		if (netp_core_set_rmem_default(_g_core_rmem_default)) {
			printf("<core> set rmem default %d failed\n", 
			       _g_core_rmem_default);
		}
		else {
			printf("<core> set new rmem default %d get %d\n", 
			       _g_core_rmem_default, 
			       netp_core_get_rmem_default());
		}
	}
	
	if (_g_cmd & CORE_GET_RMEM_MAX)
		printf("<core> rmem max: %d\n", netp_core_get_rmem_max());

	if (_g_cmd & CORE_SET_RMEM_MAX) {
		if (netp_core_set_rmem_max(_g_core_rmem_max)) {
			printf("<core> set rmem max %d failed\n", 
			       _g_core_rmem_max);
		}
		else {
			printf("<core> set new rmem max %d get %d\n", 
			       _g_core_rmem_max, 
			       netp_core_get_rmem_max());
		}
	}
	
	if (_g_cmd & CORE_GET_WMEM_DEFAULT)
		printf("<core> wmem default: %d\n", 
		       netp_core_get_wmem_default());

	if (_g_cmd & CORE_SET_WMEM_DEFAULT) {
		if (netp_core_set_wmem_default(_g_core_wmem_default)) {
			printf("<core> set wmem default %d failed\n", 
			       _g_core_wmem_default);
		}
		else {
			printf("<core> set new wmem default %d get %d\n", 
			       _g_core_wmem_default, 
			       netp_core_get_wmem_default());
		}
	}
	

	if (_g_cmd & CORE_GET_WMEM_MAX)
		printf("<core> wmem max: %d\n", 
		       netp_core_get_wmem_max());

	if (_g_cmd & CORE_SET_WMEM_MAX) {
		if (netp_core_set_wmem_max(_g_core_wmem_max)) {
			printf("<core> set wmem max %d failed\n", 
			       _g_core_wmem_max);
		}
		else {
			printf("<core> set new wmem max %d get %d\n", 
			       _g_core_wmem_max, 
			       netp_core_get_wmem_max());
		}
	}
	

	if (_g_cmd & CORE_GET_SOMAXCONN)
		printf("<core> somaxconn is: %d\n", 
		       netp_core_get_somaxconn());

	if (_g_cmd & CORE_SET_SOMAXCONN) {
		if (netp_core_set_somaxconn(_g_core_somaxconn)) {
			printf("<core> set somaxconn %d failed\n", 
			       _g_core_somaxconn);
		}
		else {
			printf("<core> set new somaxconn %d get %d\n", 
			       _g_core_somaxconn, 
			       netp_core_get_somaxconn());
		}
	}
	
	if (_g_cmd & CORE_GET_RPS)
		printf("<core> rps %d\n", netp_core_get_rps());

	if (_g_cmd & CORE_SET_RPS) {
		if (netp_core_set_rps(_g_core_rps)) {
			printf("<core> set rps %d failed\n", 
			       _g_core_rps);
		}
		else {
			printf("<core> set new rps %d get %d\n", 
			       _g_core_rps, 
			       netp_core_get_rps());
		}
	}
       
	return 0;
}

static int 
_ip4_test(void)
{
	char local[32];

	if (_g_cmd & IP4_GET_FORWARD)
		printf("<ip4> forward: %d\n", netp_ip4_get_forward());

	if (_g_cmd & IP4_SET_FORWARD) {
		if (netp_ip4_set_forward(_g_ip4_forward)) {
			printf("<ip4> set forward %d failed\n", 
			       _g_ip4_forward);
		}
		else {
			printf("<ip4> set new forward %d get %d\n", 
			       _g_ip4_forward, 
			       netp_ip4_get_forward());
		}
	}

	if (_g_cmd & IP4_GET_LOCAL_PORT_RANGE)
		printf("<ip4> local port range: %s\n", 
		       netp_ip4_get_local_port_range(local, 32));

	if (_g_cmd & IP4_SET_LOCAL_PORT_RANGE) {
		if (netp_ip4_set_local_port_range(_g_ip4_local_port_range)) {
			printf("<ip4> set local port range %s failed\n", 
			       _g_ip4_local_port_range);
		}
		else {
			printf("<ip4> set new local port range %s get %s\n", 
			       _g_ip4_local_port_range, 
			       netp_ip4_get_local_port_range(local, 32));
		}
	}

	if (_g_cmd & IP4_GET_PROMOTE_SECONDARIES) {
		if (strlen(_g_ifname) < 1) {
			printf("<ip4> promote secondaries need interface name\n");
			return -1;
		}		
		printf("<ip4> %s promote_secondaries: %d\n", _g_ifname, 
		       netp_ip4_get_promote_secondaries(_g_ifname));
	}
	if (_g_cmd & IP4_SET_PROMOTE_SECONDARIES) {
		if (strlen(_g_ifname) < 1) {
			printf("<ip4> promote secondaries need interface name\n");
			return -1;
		}
		if (netp_ip4_set_promote_secondaries(
			    _g_ifname, _g_ip4_promote_secondaries)) 
		{
			printf("<ip4> set promote secondaries %d failed\n", 
			       _g_ip4_promote_secondaries);
		}
		else {
			printf("<ip4> set new promote_secondaries %d get %d\n", 
			       _g_ip4_promote_secondaries, 
			       netp_ip4_get_promote_secondaries(_g_ifname));
		}
	}

	return 0;
}

static int 
_ip6_test(void)
{
	if (_g_cmd & IP6_GET_ACCEPT_DAD)
		printf("<ip6> %s accept dad: %d\n", _g_ifname, 
		       netp_ip6_get_accept_dad(_g_ifname));

	if (_g_cmd & IP6_SET_ACCEPT_DAD) {
		if (netp_ip6_set_accept_dad(_g_ifname, _g_ip6_accept_dad)) {
			printf("<ip6> set %s accept_dad %d failed\n", 
			       _g_ifname, _g_ip6_accept_dad);
		}
		else {
			printf("<ip6> set %s new accept_dad %d get %d\n", 
			       _g_ifname, _g_ip6_accept_dad, 
			       netp_ip6_get_accept_dad(_g_ifname));
		}
	}
	
	if (_g_cmd & IP6_GET_FORWARDING)
		printf("<ip6> %s forwarding: %d\n", _g_ifname, 
		       netp_ip6_get_forwarding(_g_ifname));

	if (_g_cmd & IP6_SET_FORWARDING) {
		if (netp_ip6_set_forwarding(_g_ifname, _g_ip6_forwarding)) {
			printf("<ip6> set %s forwarding %d failed\n", 
			       _g_ifname, _g_ip6_forwarding);
		}
		else {
			printf("<ip6> set %s new forwarding %d get %d\n", 
			       _g_ifname, _g_ip6_forwarding, 
			       netp_ip6_get_forwarding(_g_ifname));
		}
	}

	if (_g_cmd & IP6_GET_MTU)
		printf("<ip6> %s mtu: %d\n", _g_ifname, 
		       netp_ip6_get_mtu(_g_ifname));

	if (_g_cmd & IP6_SET_MTU) {
		if (netp_ip6_set_mtu(_g_ifname, _g_ip6_mtu)) {
			printf("<ip6> set %s mtu %d failed\n", 
			       _g_ifname, _g_ip6_mtu);
		}
		else {
			printf("<ip6> set %s new mtu %d get %d\n", 
			       _g_ifname, _g_ip6_mtu, 
			       netp_ip6_get_mtu(_g_ifname));
		}
	}

	if (_g_cmd & IP6_GET_DISABLE)
		printf("<ip6> %s disable: %d\n", _g_ifname, 
		       netp_ip6_get_forwarding(_g_ifname));

	if (_g_cmd & IP6_SET_DISABLE) {
		if (netp_ip6_set_disable(_g_ifname, _g_ip6_disable)) {
			printf("<ip6> set %s disable %d failed\n", 
			       _g_ifname, _g_ip6_disable);
		}
		else {
			printf("<ip6> set %s new disable %d get %d\n", 
			       _g_ifname, _g_ip6_disable, 
			       netp_ip6_get_disable(_g_ifname));
		}
	}

	return 0;
}

static int 
_tcp_test(void)
{
	char mem[32];
	size_t size;

	size = sizeof(mem);

	if (_g_cmd & TCP_GET_MEM)
		printf("<tcp> mem: %s\n", 
		       netp_tcp4_get_mem(mem, size));

	if (_g_cmd & TCP_SET_MEM) {
		if (netp_tcp4_set_mem(_g_tcp_mem)) {
			printf("<tcp> set mem %s failed\n", 
			       _g_tcp_mem);
		}
		else {
			printf("<tcp> set new mem %s get %s\n", 
			       _g_tcp_mem, 
			       netp_tcp4_get_mem(mem, size));
		}
	}

	if (_g_cmd & TCP_GET_RMEM)
		printf("<tcp> rmem: %s\n", 
		       netp_tcp4_get_rmem(mem, size));

	if (_g_cmd & TCP_SET_RMEM) {
		if (netp_tcp4_set_rmem(_g_tcp_rmem)) {
			printf("<tcp> set rmem %s failed\n", 
			       _g_tcp_rmem);
		}
		else {
			printf("<tcp> set new rmem %s get %s\n", 
			       _g_tcp_rmem, 
			       netp_tcp4_get_rmem(mem, size));
		}
	}

	if (_g_cmd & TCP_GET_WMEM)
		printf("<tcp> wmem: %s\n", 
		       netp_tcp4_get_wmem(mem, size));

	if (_g_cmd & TCP_SET_WMEM) {
		if (netp_tcp4_set_wmem(_g_tcp_wmem)) {
			printf("<tcp> set wmem %s failed\n", 
			       _g_tcp_wmem);
		}
		else {
			printf("<tcp> set new wmem %s get %s\n", 
			       _g_tcp_wmem, 
			       netp_tcp4_get_wmem(mem, size));
		}
	}
	
	if (_g_cmd & TCP_GET_TIMESTAMPS)
		printf("<tcp> timestamps: %d\n", 
		       netp_tcp4_get_timestamps());

	if (_g_cmd & TCP_SET_TIMESTAMPS) {
		if (netp_tcp4_set_timestamps(_g_tcp_timestamps)) {
			printf("<tcp> set timestamps %d failed\n", 
			       _g_tcp_timestamps);
		}
		else {
			printf("<tcp> set new timestamps %d get %d\n", 
			       _g_tcp_timestamps, 
			       netp_tcp4_get_timestamps());
		}
	}

	if (_g_cmd & TCP_GET_TW_REUSE)
		printf("<tcp> tw_reuse: %d\n", 
		       netp_tcp4_get_tw_reuse());

	if (_g_cmd & TCP_SET_TW_REUSE) {
		if (netp_tcp4_set_tw_reuse(_g_tcp_tw_reuse)) {
			printf("<tcp> set tw_reuse %d failed\n", 
			       _g_tcp_tw_reuse);
		}
		else {
			printf("<tcp> set new tw_reuse %d get %d\n", 
			       _g_tcp_tw_reuse, 
			       netp_tcp4_get_tw_reuse());
		}
	}

	if (_g_cmd & TCP_GET_TW_RECYCLE)
		printf("<tcp> tw_recycle: %d\n", 
		       netp_tcp4_get_tw_recycle());

	if (_g_cmd & TCP_SET_TW_RECYCLE) {
		if (netp_tcp4_set_tw_recycle(_g_tcp_tw_recycle)) {
			printf("<tcp> set tw_recycle %d failed\n", 
			       _g_tcp_tw_recycle);
		}
		else {
			printf("<tcp> set new tw_recycle %d get %d\n", 
			       _g_tcp_tw_recycle, 
			       netp_tcp4_get_tw_recycle());
		}
	}

	if (_g_cmd & TCP_GET_FIN_TIMEOUT)
		printf("<tcp> fin_timeout: %d\n", 
		       netp_tcp4_get_fin_timeout());

	if (_g_cmd & TCP_SET_FIN_TIMEOUT) {
		if (netp_tcp4_set_fin_timeout(_g_tcp_fin_timeout)) {
			printf("<tcp> set fin_timeout %d failed\n", 
			       _g_tcp_fin_timeout);
		}
		else {
			printf("<tcp> set new fin_timeout %d get %d\n", 
			       _g_tcp_fin_timeout, 
			       netp_tcp4_get_fin_timeout());
		}
	}

	if (_g_cmd & TCP_GET_MAX_SYN_BACKLOG)
		printf("<tcp> max_syn_backlog: %d\n", 
		       netp_tcp4_get_max_syn_backlog());

	if (_g_cmd & TCP_SET_MAX_SYN_BACKLOG) {
		if (netp_tcp4_set_max_syn_backlog(_g_tcp_max_syn_backlog)) {
			printf("<tcp> set max_syn_backlog %d failed\n", 
			       _g_tcp_max_syn_backlog);
		}
		else {
			printf("<tcp> set new max_syn_backlog %d get %d\n", 
			       _g_tcp_max_syn_backlog, 
			       netp_tcp4_get_max_syn_backlog());
		}
	}

	if (_g_cmd & TCP_GET_MAX_TW_BUCKETS)
		printf("<tcp> max_tw_buckets: %d\n", 
		       netp_tcp4_get_max_tw_buckets());

	if (_g_cmd & TCP_SET_MAX_TW_BUCKETS) {
		if (netp_tcp4_set_max_tw_buckets(_g_tcp_max_tw_buckets)) {
			printf("<tcp> set max_tw_buckets %d failed\n", 
			       _g_tcp_max_tw_buckets);
		}
		else {
			printf("<tcp> set new max_tw_buckets %d get %d\n", 
			       _g_tcp_max_tw_buckets, 
			       netp_tcp4_get_max_tw_buckets());
		}
	}

	return 0;
}

static int 
_irq_get_intf_irqs(intf_irq_t *irq)
{
	int n, i;
	int irqs[IRQ_MAX_IRQ];
	char irqname[32];
	char ifname[32];

	n = nets_get_irqs(irq->name, irqs, IRQ_MAX_IRQ);
	if (n <= 0)
		return -1;

	if (n == 1) {
		irq->irqs[0] = irqs[0];
		irq->nirq = 1;
		return 0;
	}		

	/* check it's control IRQ or receive IRQ */
	for (i = 0; i < n; i++) {
		if (netp_irq_get_name(irqs[i], irqname, sizeof(irqname)) == NULL)
			return -1;

		/* if name is portN, It's control IRQ */
		if (strcmp(irq->name, irqname) == 0)
			continue;
	       
		/* if name is portN-0, it's TX port */
		snprintf(ifname, sizeof(ifname), "%s-0", irq->name);
		if (strcmp(ifname, irqname) == 0)
			continue;

		/* if name is portN-tx, it's TX IRQ */
		snprintf(ifname, sizeof(ifname), "%s-tx", irq->name);
		if (strncmp(ifname, irqname, strlen(ifname)) == 0)
			continue;

		irq->irqs[irq->nirq] = irqs[i];
		irq->nirq++;
	}

	/* set interface max IRQs */
	if (irq->nirq > _g_irq_max)
		_g_irq_max = irq->nirq;

	return 0;
}

static int 
_irq_get_mask(void)
{
	int i, j;
	int ncpu;
	int percpu;
	u_int32_t mask;
	u_int32_t mask1;
	u_int32_t mask2;
	int index = 0;
	int start;
	int mod;

	ncpu = cpu_get_number();
	if (ncpu < 2)
		return -1;

	/* get CPU start/mod */
	if (_g_irq_ht == IRQ_HT_FULL) {
		start = 0;
		mod = ncpu;
	}
	else if (_g_irq_ht == IRQ_HT_LOW) {
		start = 0;
		mod = ncpu / 2;
	}
	else if (_g_irq_ht == IRQ_HT_HIGH) {
		start = ncpu / 2;
		mod = ncpu / 2;
	}
	else {
		return -1;
	}

	switch (_g_irq_algo) {

	case IRQ_BIND_RR:
		index = start;
		for (i = 0; i < _g_irq_nintf; i++) {
			for (j = 0; j < _g_irq_intfs[i].nirq; j++) {
				mask = 1 << index;
				_g_irq_intfs[i].masks[j] = mask;
				index = (index + 1) % mod + start;
			}
		}
		
		break;
	
	case IRQ_BIND_ODD:
		index = start;
		for (i = 0; i < _g_irq_nintf; i++) {
			for (j = 0; j < _g_irq_intfs[i].nirq; j++) {
				mask = 1 << index;
				_g_irq_intfs[i].masks[j] = mask;
				index = (index + 2) % mod + start;
			}
		}
		break;

	case IRQ_BIND_EVEN:
		index = start + 1;
		for (i = 0; i < _g_irq_nintf; i++) {
			for (j = 0; j < _g_irq_intfs[i].nirq; j++) {
				mask = 1 << index;
				_g_irq_intfs[i].masks[j] = mask;
				index = (index + 2) % mod + start;
			}
		}
		break;

	case IRQ_BIND_FULL:
		for (i = 0; i < _g_irq_nintf; i++) {
			percpu = ncpu/_g_irq_intfs[i].nirq;
			if (percpu < 1)
				percpu = 1;
			for (j = 0; j < _g_irq_intfs[i].nirq; j++) {
				mask1 = 0xffffffff << index;
				mask2 = 0xffffffff << (index + percpu);
				_g_irq_intfs[i].masks[j] = mask1 ^ mask2;
				index = (index + percpu) % ncpu;			
			}
		}
		break;


	default:
		return -1;
	}

	return 0;
}

static int 
_irq_test(void)
{
	int i, j;
	int irq;
	u_int32_t mask;

	/* get all interface IRQs */
	for (i = 0; i < _g_irq_nintf; i++) {
		if (_irq_get_intf_irqs(&_g_irq_intfs[i]))
		{
			printf("<irq> %s get irqs failed: %s\n", 
			       _g_irq_intfs[i].name, netp_get_error());
			return -1;
		}
	}

	if (_g_cmd & IRQ_GET_CPU) {
		for (i = 0; i < _g_irq_nintf; i++) {
			printf("<irq> %s:\n", _g_irq_intfs[i].name);
			for (j = 0; j < _g_irq_intfs[i].nirq; j++) {
				irq = _g_irq_intfs[i].irqs[j];
				mask = netp_irq_get_cpu(irq);
				printf("\t%d: %.8x\n", irq, mask);
			}
		}
	}

	if (_g_cmd & IRQ_SET_CPU) {
		_irq_get_mask();
		for (i = 0; i < _g_irq_nintf; i++) {
			for (j = 0; j < _g_irq_intfs[i].nirq; j++) {
				irq = _g_irq_intfs[i].irqs[j];
				mask = _g_irq_intfs[i].masks[j];
				if (netp_irq_set_cpu(irq, mask)) {
					printf("<irq> %d set cpu %.8x failed: %s\n", 
					       irq, mask, netp_get_error());
				}
				printf("\t%d: %.8x\n", irq, mask);
			}
		}
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

	case TEST_SYSFS:
		_sysfs_test();
		break;
	case TEST_CORE:
		_core_test();
		break;
	case TEST_IP4:
		_ip4_test();
		break;
	case TEST_IP6:
		_ip6_test();
		break;
	case TEST_TCP:
		_tcp_test();
		break;
	case TEST_IRQ:
		_irq_test();
	default:
		break;
	}

	_release();

	return 0;
}



/**
 *	@file	
 *
 *	@brief
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */






