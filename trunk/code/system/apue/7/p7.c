/*
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>

#define	do_it(name)	pr_limit(#name, name)

static void pr_limit(char* name, int resource);

int main(void)
{
    do_it(RLIMIT_CPU);
    do_it(RLIMIT_FSIZE);
    do_it(RLIMIT_DATA);
    do_it(RLIMIT_STACK);
    do_it(RLIMIT_CORE);
    do_it(RLIMIT_RSS);
    do_it(RLIMIT_NPROC);
    do_it(RLIMIT_NOFILE);
    do_it(RLIMIT_MEMLOCK);
    do_it(RLIMIT_AS);
    do_it(RLIMIT_LOCKS);

    exit(0);
}

static void pr_limit(char* name, int resource)
{
    struct rlimit	limit;
    
    if (getrlimit(resource, &limit) < 0) {
	printf("getrlimit error for %s", name);
	perror(NULL);
	return;
    }

    printf("%-14s ", name);
    if (limit.rlim_cur == RLIM_INFINITY)
	printf("(infinite) ");
    else
	printf("%10ld ", limit.rlim_cur);

    if (limit.rlim_max == RLIM_INFINITY)
	printf("(infinite)\n");
    else
	printf("%10ld\n", limit.rlim_max);

}

