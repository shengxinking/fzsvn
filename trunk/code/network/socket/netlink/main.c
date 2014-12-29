/*
 *  this program can add/delete/show route table using netlink
 *
 *  write by Forrest.zhang
 */

#include <stdio.h>
#include <string.h>
#include "route.h"

static void usage(void)
{
    printf("\nroute ( add | del | show )\n\n");
    printf("\tadd\t\tadd a route rule\n");
    printf("\tdel\t\tdelete a route rule\n");
    printf("\tshow\t\tshow route table\n");
    printf("\n");
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
	usage();
	return -1;
    }

    if (strcmp(argv[1], "add") == 0)
	return add_route(argc, argv);
    else if (strcmp(argv[1], "del") == 0)
	return del_route(argc, argv);
    else if (strcmp(argv[1], "show") == 0)
	return show_route(argc, argv);
    else
	usage();

    return -1;
}
