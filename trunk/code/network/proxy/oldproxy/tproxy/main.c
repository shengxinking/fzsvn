/**
 *	@file	main.c
 *	@brief	the main function of tcpproxy, it read command line parameter
 *		then run a proxy. 
 * 
 *	@author	FZ
 */


#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "debug.h"
#include "proxy.h"

static proxy_arg_t	_g_arg;		/* the proxy argument */

static void 
_usage(void)
{
	printf("tproxy <options>\n");
	printf("\t-p\t\tpolicy address: <vs>,<rs>[,<rs>...]\n");
	printf("\t\t\t<vs> is listen address: <IP:port>\n");
	printf("\t\t\t<rs> is real server address: <IP:port>\n");
	printf("\t-w <N>\t\twork thread number\n");
	printf("\t-m <N>\t\tproxy max concurrency\n");
	printf("\t-b <algo>\tbind CPU, algorithm is: rr|odd|even\n");
	printf("\t-r <range>\tHT CPU range: full|low|high\n");
	printf("\t-d <0-4>\tthe debug level<0-4>, 0 is disabled\n");
	printf("\t-s\t\tuse splice\n");
	printf("\t-n\t\tuse nb_splice\n");
	printf("\t-h\t\tshow help usage\n");
}

static int 
_parse_policy(char *addr)
{
	policy_t *pl;
	char *ptr;
	char *begin;
	int i;

	if (!addr)
		return -1;

	if (_g_arg.npolicy >= MAX_POLICY) {
		printf("too many policies, exceed %d\n", MAX_POLICY);
		return -1;
	}

	pl = &_g_arg.policies[_g_arg.npolicy];

	begin = addr;
	ptr = strchr(begin, ',');
	if (!ptr) {
		printf("miss real server address\n");
		return -1;
	}

	/* get listen address */
	*ptr = 0;
	if (ip_port_from_str(&pl->httpaddr, begin)) {
		printf("invalid listen address %s\n", addr);
		return -1;
	}

	/* check exist real server address */
	begin = ptr + 1;
	if (*begin == 0) {
		printf("not real server address\n");
		return -1;
	}

	i = 0;
	while (1) {
		ptr = strchr(begin, ',');

		/* last real server address */
		if (ptr == NULL) {
			if (ip_port_from_str(&pl->rsaddrs[i], begin)) {
				printf("invalid real server address %s", begin);
				return -1;
			}
			i++;
			break;
		}

		*ptr = 0;
		if (ip_port_from_str(&pl->rsaddrs[i], begin)) {
			printf("invalid real server address %s", begin);
			return -1;
		}
		i++;
		begin = ptr + 1;
		if (*begin == 0)
			break;
	}

	if (i == 0) {
		printf("no real server address\n");
		return -1;
	}

	pl->nrsaddr = i;
	pl->index = _g_arg.npolicy;
	_g_arg.npolicy++;

	return 0;
}

static int 
_parse_cmd(int argc, char **argv)
{
	char c;
	char optstr[] = ":p:m:w:b:r:d:snh";
	char *addr;

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {

		case 'p':
			addr = strdup(optarg);
			if (_parse_policy(addr)) {
				printf("invalid policy: %s\n", optarg);
				free(addr);
				return -1;
			}
			free(addr);
			break;

		case 'm':
			_g_arg.max = atoi(optarg);
			if (_g_arg.max < 1) {
				printf("invalid capacity: %s\n", optarg);
				return -1;
			}
			break;

		case 'w':
			_g_arg.nworker = atoi(optarg);
			if (_g_arg.nworker < 1 || _g_arg.nworker > MAX_WORKER) {
				printf("invalid work number: %s\n", 
				       optarg);
				return -1;
			}
			break;

		case 'b':
			if (strcmp(optarg, "rr") == 0)
				_g_arg.bind_cpu_algo = THREAD_BIND_RR;
			else if (strcmp(optarg, "odd") == 0)
				_g_arg.bind_cpu_algo = THREAD_BIND_ODD;
			else if (strcmp(optarg, "even") == 0)
				_g_arg.bind_cpu_algo = THREAD_BIND_EVEN;
			else {
				printf("invalid bind CPU algorithm %s\n", optarg);
				return -1;
			}
			_g_arg.bind_cpu = 1;
			break;

		case 'r':
			if (strcmp(optarg, "full") == 0)
				_g_arg.bind_cpu_ht = THREAD_HT_FULL;
			else if (strcmp(optarg, "low") == 0)
				_g_arg.bind_cpu_ht = THREAD_HT_LOW;
			else if (strcmp(optarg, "high") == 0)
				_g_arg.bind_cpu_ht = THREAD_HT_HIGH;
			else {
				printf("invalid bind CPU ht %s\n", optarg);
				return -1;
			}
			_g_arg.bind_cpu = 1;
			break;

		case 'd':
			_g_arg.dbglvl = atoi(optarg);
			if (_g_arg.dbglvl < 0 || _g_arg.dbglvl > 4) {
				printf("invalid debug level: %s\n", optarg);
				return -1;
			}
			break;

		case 's':
			_g_arg.use_splice = 1;
			break;

		case 'n':
			_g_arg.use_nb_splice = 1;
			break;

		case 'h':
			_usage();
			exit(0);

		case ':':
			printf("%c need argument\n", optopt);
			return -1;

		case '?':
			printf("unkowned option: %c\n", optopt);
			return -1;
		}
	}

	if (optind != argc) {
		return -1;
	}
	
	if (_g_arg.npolicy < 1) {
		printf("need using -p add policy\n");
		return -1;
	}

	if (_g_arg.use_splice && _g_arg.use_nb_splice) {
		printf("the splice and nb_splice can set only one\n");
		return -1;
	}

	if (_g_arg.nworker == 0)
		_g_arg.nworker = 1;

	if (_g_arg.max == 0) 
		_g_arg.max = 100;

	if (_g_arg.use_splice)
		_g_arg.maxfd = _g_arg.max*4+_g_arg.nworker*_g_arg.npolicy+10;
	else 
		_g_arg.maxfd = _g_arg.max*2+_g_arg.nworker*_g_arg.npolicy+10;

	g_dbglvl = _g_arg.dbglvl;

	return 0;
}

int main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	proxy_main(&_g_arg);

	return 0;
}




