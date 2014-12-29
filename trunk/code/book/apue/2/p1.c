/*
 *  write by jbug
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static void pr_sysconf(const char*, int);
static void pr_pathconf(const char*, const char*, int);

int main(int argc, char* argv[])
{
    if (argc != 2) {
	printf ("usage: %s <dirname>\n", argv[0]);
	exit(1);
    }

    pr_sysconf("ARG_MAX                =", _SC_ARG_MAX);
    pr_sysconf("CHILD_MAX              =", _SC_CHILD_MAX);
    pr_sysconf("clock ticks/second     =", _SC_CLK_TCK);
    pr_sysconf("NGROUPS_MAX            =", _SC_NGROUPS_MAX);
    pr_sysconf("OPEN_MAX               =", _SC_OPEN_MAX);
    pr_sysconf("STREAM_MAX             =", _SC_STREAM_MAX);
    pr_sysconf("TZNAME_MAX             =", _SC_TZNAME_MAX);
    pr_sysconf("_POSIX_JOB_CONTROL     =", _SC_JOB_CONTROL);
    pr_sysconf("_POSIX_SAVED_IDS       =", _SC_SAVED_IDS);
    pr_sysconf("_POSIX_VERSION         =", _SC_VERSION);
    pr_sysconf("_XOPEN_VERSION         =", _SC_XOPEN_VERSION);
    
    pr_pathconf("MAX_INPUT             =", argv[1], _PC_MAX_INPUT);
    pr_pathconf("MAX_CANON             =", argv[1], _PC_MAX_CANON);
    pr_pathconf("NAME_MAX              =", argv[1], _PC_NAME_MAX);
    pr_pathconf("PATH_MAX              =", argv[1], _PC_PATH_MAX);
    pr_pathconf("LINK_MAX              =", argv[1], _PC_LINK_MAX);
    
    exit(0);
}

void pr_sysconf(const char* msg, int name)
{
    printf("%s %ld\n", msg, sysconf(name));
}

void pr_pathconf(const char* msg, const char* path, int name)
{
    printf("%s %ld\n", msg, pathconf(path, name));
}
