/*
 *
 */

#include <sys/types.h>
#include <sys/times.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static void pr_times(clock_t, struct tms*, struct tms*);
static void do_cmd(char*);

int main(int argc, char** argv)
{
    int		    i;

    for (i = 1; i < argc; i++)
	do_cmd(argv[i]);

    exit(0);
}

static void do_cmd(char* cmd)
{
    struct tms	    stms, etms;
    clock_t	    start, end;
    int		    status;

    fprintf(stderr, "\n command: %s\n", cmd);

    if ( (start = times(&stms)) == -1)
	exit(1);

    if ( (status = system(cmd)) < 0)
	exit(1);

    if ( (end = times(&etms)) == -1)
	exit(1);

    pr_times(end - start, &stms, &etms);

    pr_exit(status);
}

static void pr_times(clock_t real, struct tms* start, struct tms* end)
{
    static long		    clktck = 100;

    fprintf(stderr, " real: %7.2f\n", real / (double) clktck);
    fprintf(stderr, " user: %7.2f\n", (end->tms_utime - start->tms_utime) / (double) clktck);
    fprintf(stderr, " sys: %7.2f\n", (end->tms_stime - start->tms_stime) / (double) clktck);
    fprintf(stderr, " child user: %7.2f\n", (end->tms_cutime - start->tms_cutime) / (double) clktck);
    fprintf(stderr, " child sys: %7.2f\n", (end->tms_cstime - start->tms_cstime) / (double) clktck);
}
