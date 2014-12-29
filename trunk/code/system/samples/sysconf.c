/*
 * 	the example of sysconf function
 *	write by Forrest.zhang
 */

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

static inline const char *limit_name(int name)
{
	switch (name) {
	case _SC_ARG_MAX:
		return "Max arguments when call exec()";
	case _SC_CHILD_MAX:
		return "Max simultaneous processes per user ID";
	case _SC_HOST_NAME_MAX:
		return "Max length of a hostname";
	case _SC_LOGIN_NAME_MAX:
		return "Max length of a login name";
	case _SC_CLK_TCK:
		return "Number of clock ticks per second";
	case _SC_OPEN_MAX:
		return "Max files a process can open";
	case _SC_PAGESIZE:
		return "Size of a page in bytes";
	case _SC_RE_DUP_MAX:
		return "Number of repeated occurrences of a BRE";
	case _SC_STREAM_MAX:
		return "Max streams a process can open";
	case _SC_TTY_NAME_MAX:
		return "Max length of a terminal device name";
	case _SC_TZNAME_MAX:
		return "Max length of timezone name";
	case _SC_VERSION:
		return "POSIX.1 year and month";
	default:
		return "Unkowned option";
	}
}

static void inline print_limit(int name)
{
	long ret = 0;

	ret = sysconf(name);
	if (ret < 0)
		printf("[%s]: %s\n", __FUNCTION__, strerror(errno));
	else
		printf("%s value is %ld\n", limit_name(name), ret);
}

int main(void)
{
	print_limit(_SC_ARG_MAX);
	print_limit(_SC_CHILD_MAX);
	print_limit(_SC_HOST_NAME_MAX);
	print_limit(_SC_LOGIN_NAME_MAX);
	print_limit(_SC_CLK_TCK);
	print_limit(_SC_OPEN_MAX);
	print_limit(_SC_PAGESIZE);
	print_limit(_SC_RE_DUP_MAX);
	print_limit(_SC_STREAM_MAX);
	print_limit(_SC_TTY_NAME_MAX);
	print_limit(_SC_TZNAME_MAX);
//	print_limit(_SC_);
	print_limit(_SC_VERSION);

	return 0;
}
