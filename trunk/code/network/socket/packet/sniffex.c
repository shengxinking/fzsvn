/*
 *	@file	sniffex.c
 *
 *	@brief	a simple sniffer program using LSF(linux socket filter).
 *		see kernel document: Documentation/networking/filter.txt
 *		and BPF(Berkley Packet filter)
 *
 *	@author	Forrest.zhang
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/filter.h>


static void _usage(void)
{
	printf("sniffex <options>\n");
	printf("\t-h\tshow help message\n");
}

static int _parse_cmd(int argc, char **argv)
{
	return 0;
}

static int _initiate(void)
{
	return 0;
}

static int _release(void)
{
	return 0;
}


static int _process(void)
{

}


int main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate())
		return -1;

	_process();

	_release();
	
	return 0;
}


