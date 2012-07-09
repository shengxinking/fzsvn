/*
 *	test waitpid and it status
 *	
 *	write by Forrest.zhang
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

static void _usage(void)
{
	printf("waitpid <child exit status>\n");
}

int main(int argc, char **argv)
{
	pid_t pid;
	int exit_status = 0;
	int status = 0;
	
	if (argc != 2) {
		_usage();
		return 0;
	}

	exit_status = atoi(argv[1]);

	if ( (pid = fork()) == 0) {
		return (exit_status);
	}
	else if (pid > 0) {
		usleep(5000);
		exit_status = 0;
		waitpid(0, &status, WNOHANG);
		if (WIFEXITED(status)) {
			exit_status = WEXITSTATUS(status);
			printf("child exit value is %d\n", exit_status);
		}

		return 0;
	}
	else {
		printf("Can't fork child\n");
	}
}






