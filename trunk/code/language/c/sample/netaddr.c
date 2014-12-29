/*
 *  this is a test program for getnetbyname, getnetbyaddr
 *
 *  write by Forrest.zhang
 */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>

int
main(int argc, char** argv)
{
    struct netent*      entry = NULL;
    int                 i = 0;

    if (argc != 2)
	return -1;

    entry = getnetbyname(argv[1]);
    if (entry) {
	printf("network name: %s\n", entry->n_name);
	
	if (entry->n_aliases) {
	    printf("network aliases:\n");
	    while (1) {
		if (entry->n_aliases[i]) {
		    printf("\t%s\n", entry->n_aliases[i]);
		    ++i;
		}
		else
		    break;
	    }
	}
	
	switch (entry->n_addrtype) {
	case AF_INET:
	    printf("network address type: IPV4\n");
	    break;
	case AF_INET6:
	    printf("network address type: IPV6\n");
	    break;
	default:
	    printf("unkowned network address type\n");
	}
	
	return 0;
    }
    else {
	perror("error: ");
	return -1;
    }
}
