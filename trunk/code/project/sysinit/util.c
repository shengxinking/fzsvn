/**
 *	@file	util.c
 *
 *	@brief	The utils API for init program
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2010-08-30
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/ioctl.h>


/**
 *	Sleep @msec microsecond.
 *
 *	No return.
 */
void 
ut_mdelay(int msec)
{
	struct timespec tm;

	tm.tv_sec = msec/1000;
	tm.tv_nsec = (msec - tm.tv_sec * 1000) * 1000000;
	while (nanosleep(&tm, &tm) == -1 && errno == EINTR)
		;
}

/**
 *	Wait the process @pid exit.
 *
 *	Return the process return value.
 */
extern int 
ut_waitfor(pid_t pid)
{
	int status = 0;
        pid_t wpid;

        if (pid < 0)
                return 0;

        while (1) {
                wpid = waitpid(pid, &status, 0);
                if (wpid < 0) {

                        if (errno == EINTR)
                                continue;

                        printf("wait for %d error: %s\n", pid, strerror(errno));
                        status = - errno;
                        return status;
                }
                else if (wpid == 0)
                        return 0;
                else
                        return WEXITSTATUS(status);
        }

	return status;
}


/**
 *	Run the command and wait the return value.
 *
 *	Return the program return value.
 */
int
ut_runcmd(const char *cmd)
{
	int status = 0;
        pid_t pid;

        pid = fork();
        if (pid < 0) {
                printf("fork() failed\n");
                return -1;
        }
        else if (pid == 0) {
                char *cmdpath, *s, *ptr;
                char *envp[] = { "TERM=vt100", "PATH=/bin:/sbin", NULL };
                char *args[255];
                int i;

                /* reset signal handler */
                for (i = 1; i < 32; i++)
                        signal(i, SIG_DFL);	
	              ptr = strdup(cmd);
                for (s = ptr, i = 0; (s = strsep(&ptr, " \t")) != NULL;)
                        if (*s != '\0')
                                args[i++] = s++;

                args[i] = NULL;
                cmdpath = args[0];

                execve(cmdpath, args, envp);
                /* We're still here?  Some error happened. */
                printf("Can't run %s: %s\n", args[0], strerror(errno));
                exit(-1);
        }

        status = ut_waitfor(pid);

        return status;
}

static 
int ut_set_term(int fd, int isctty)
{
	struct termios tty;

	if (fd < 0)
		return -1;

	if (tcgetattr(fd, &tty) < 0) {
		printf("tcgetattr failed: %s\n", strerror(errno));
		return -1;
	}
	tty.c_cc[VINTR] = 3;	/* C-c */
	tty.c_cc[VQUIT] = 28;	/* C-\ */
	tty.c_cc[VERASE] = 127;	/* C-? */
	tty.c_cc[VKILL] = 21;	/* C-u */
	tty.c_cc[VEOF] = 4;	/* C-d */
	tty.c_cc[VSTART] = 17;	/* C-q */
	tty.c_cc[VSTOP] = 19;	/* C-s */
	tty.c_cc[VSUSP] = 26;	/* C-z */

	/* Make it be sane */
	tty.c_cflag &= CBAUD | CBAUDEX | CSIZE | CSTOPB | PARENB | PARODD;
	tty.c_cflag |= HUPCL | CLOCAL;
	
	/* input modes */
	tty.c_iflag = ICRNL | IXON | IXOFF;
	
	/* output modes */
	tty.c_oflag = OPOST | ONLCR;
	
	/* local modes */
	tty.c_lflag =  ISIG | ICANON | ECHO | ECHOE | ECHOK;
	tty.c_lflag |= ECHOCTL | ECHOKE | IEXTEN;

	if (tcsetattr(fd, TCSANOW, &tty) < 0) {
		printf("tcsetattr failed: %s\n", strerror(errno));
		return -1;
	}

	/* set conctrol TTY */
	if (isctty) {
		/* set control TTY */
		if (ioctl(fd, TIOCSCTTY, 1)) {
			printf("set control tty failed: %s\n", strerror(errno));
		}

		/* set fore-group */
		if (tcsetpgrp(fd, getpgrp()) < 0) {
			printf("set pgrp failed: %s\n", strerror(errno));
			return -1;
		}
	}

	return 0;
}

/**
 *	Set the console for init program
 *
 *	No return.
 */
int 
ut_set_console(const char *dev, int isctty)
{
	int fd;
	int i;

	/* close all fd */
	for (i = 0; i < 64; i++)
		close(i);

	fd = open(dev, O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		return -1;
	}
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);

	/* set terminal */
	ut_set_term(0, isctty);

	return 0;
}



