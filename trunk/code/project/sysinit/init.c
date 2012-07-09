/**
 *	@file	init.c
 *
 *	@brief	The system init program, the pid is 0.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2010-08-26
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/io.h>
#include <sys/reboot.h>
#include <linux/reboot.h>

#include "util.h"

#define FULL_MODE	1

/**
 *	The init program level.
 */
typedef enum {
	INIT_SYSINIT = 1, /* run onetime at system startup */
	INIT_RESPAWN,	/* run as daemon when system start up */
	INIT_RESPAWN2,	/* run as daemon, can stop/start by init */
	INIT_UPDATE,	/* run when update the configure */
	INIT_UPGRADE,	/* upgrade firmware */
} init_level_e;


/**
 *	The init entry program.
 */
typedef struct init_entry {
	int		opmode;		/* the opmode of program run */
	init_level_e	level;		/* the level */
	pid_t		pid;		/* the pid of program */
	int		status;		/* the running status */
	const char	*cmd;		/* the command */
	const char	*abbr;		/* the program name */
} init_entry_t;


init_entry_t init_apps[] = {
//	{FULL_MODE, INIT_SYSINIT, 0, 0, "/bin/smit startup", "startup"},
	{FULL_MODE, INIT_RESPAWN, 0, 0, "/bin/busybox sh", "busybox"},
	{FULL_MODE, INIT_UPGRADE, 0, 0, "/bin/smit upgrade", "upgrade"},
	{FULL_MODE, INIT_UPDATE, 0, 0, "/bin/smit update", "update"},
};

/* the init signals, we using normal signal for init. */
static char init_sigs[32];
static int  init_nsigs;


/**
 *	Run a daemon program.
 *
 *	Return the pid of daemon, 0 if error.
 */
static pid_t 
_run_daemon(const char *cmd)
{
	pid_t pid;
	int i;
	char *cmdpath, *s;
	char *c;
	char *args[255];	
	static char *envp[] = {
		"TERM=vt100",
		"PATH=/bin:/sbin",
		NULL
	};
	
	if (!cmd) {
		return 0;
	}

	pid = fork();
	if (pid < 0) {
		printf("fork() failed\n");
		return 0;
	}
	else if (pid == 0) {

		/* reset signal handler, close all fd */
		for (i = 1; i < 64; i++) {
			signal(i, SIG_DFL);
		}

		setsid();

		ut_set_console("/dev/tty8", 1);

		cmd = strdup(cmd);		
		if (cmd == NULL) {
			printf("strdup() failed\n");
			return -1;
		}
		
		i = 0;
		c = (char *)cmd;
		for (s = c; (s = strsep(&c, " \t")) != NULL;) {
			if (*s != '\0') {
				args[i] = s;
				s++;
				i++;
			}
		}

		args[i] = NULL;
		cmdpath = args[0];
	
		execve(cmdpath, args, envp);

		/* We're still here?  Some error happened. */
		printf("Can't run %s: %d\n", args[0], errno);
		exit(0);
	}

	printf("new daemon pid is %d\n", pid);

	return pid;
}


/**
 *	Stop the daemon entry @e.	
 * 
 *	No return.
 */
static void 
_stop_daemon(init_entry_t *e)
{
	kill(e->pid, SIGTERM);
	ut_mdelay(1000);
	kill(e->pid, SIGKILL);
	e->pid = 0;
}


/**
 *	Run the init entry @e.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_run_entry(init_entry_t *e)
{
	int ret = 0;

	if (!e)
		return -1;

	if (e->level == INIT_SYSINIT ||
	    e->level == INIT_UPDATE ||
	    e->level == INIT_UPGRADE)
	{
		ret = ut_runcmd(e->cmd);
		return ret;
	}

	if (e->level == INIT_RESPAWN) {
		if (e->pid > 1)
			return 0;
		ret = _run_daemon(e->cmd);
		if (ret < 1)
			return -1;
		else {
			e->pid = ret;
			return 0;
		}
	}

	if (e->level == INIT_RESPAWN2) {
		if (e->pid > 1 && e->status == 0) {
			_stop_daemon(e);
			return 0;
		}
		
		if (e->pid < 1 && e->status == 1) {
			ret = _run_daemon(e->cmd);
			if (ret > 1) {
				e->pid = ret;
				return 0;
			}
			else {
				return 1;
			}
		}
	}

	return 0;
}



/**
 *	Run the all entries which level is @level in @init_apps
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_run_level(init_level_e level)
{
	int i;
	int napps;
	int ret;
	
	napps = sizeof(init_apps)/sizeof(init_entry_t);

	switch (level) {

	case INIT_SYSINIT:
		for (i = 0; i < napps; i++) {
			if (init_apps[i].level != level)
				continue;

			printf("_run_entry(sysinit) %s\n", init_apps[i].cmd);

			ret = ut_runcmd(init_apps[i].cmd);
			if (ret != 0) {
				printf("run (%s) failed\n", 
				       init_apps[i].abbr);
				pause();
			}
		}
		
		break;

	case INIT_RESPAWN:
		for (i = 0; i < napps; i++) {
			if (init_apps[i].level != level ||
			    init_apps[i].pid > 1)
				continue;
			
			printf("_run_entry(respawn) %s\n", init_apps[i].cmd);
			ret = _run_entry(&init_apps[i]);
			if (ret) {
				printf("run (%s) failed\n", 
				       init_apps[i].abbr);
				pause();
			}
		}
		
		break;

	case INIT_RESPAWN2:
		for (i = 0; i < napps; i++) {
			if (init_apps[i].level != level)
				continue;

			ret = _run_entry(&init_apps[i]);
			if (ret < 1) {
				printf("run (%s) failed\n", 
				       init_apps[i].abbr);
			}
			else {
				init_apps[i].pid = ret;
			}
		}
		break;

	default:

		printf("unkowned level\n");
	}

	return 0;
}

static int 
_stop_level(init_level_e level)
{
	int i;
	int napps;
	
	napps = sizeof(init_apps)/sizeof(init_entry_t);

	switch (level) {

	case INIT_RESPAWN:
		for (i = 0; i < napps; i++) {
			if (init_apps[i].level != level ||
			    init_apps[i].pid < 1)
				continue;
			
			_stop_daemon(&init_apps[i]);
		}
		
		break;

	case INIT_RESPAWN2:
		for (i = 0; i < napps; i++) {
			if (init_apps[i].level != level ||
			    init_apps[i].pid < 1)
				continue;

			_stop_daemon(&init_apps[i]);
		}
		break;

	default:

		printf("unkowned level\n");
	}

	return 0;
}


/**
 *	Reset CPU, it fixed bug when unit reboot, the boot sequence changed.
 *
 *	No return.
 */
static void 
_reset_cpu(void)
{
	/* set io previlege */
	iopl(3);
	
	/* reboot MP cpu */
	outb(0x02, 0xcf9);
	outb(0x06, 0xcf9);
	outb(0x02, 0xcf9);
	outb(0x0e, 0xcf9);
}


static void 
_do_reboot(void)
{
	int i;

	ut_mdelay(200);

	reboot(RB_ENABLE_CAD);

	printf("\r\n\r\nThe system is going down NOW !!\r\n");

	/* send the TERM and KILL signal */
	kill(-1, SIGTERM);
	ut_mdelay(2000);
	kill(-1, SIGKILL);

	for (i = 0; init_apps[i].level != 0; i++)
		init_apps[i].pid = 0;
	sync();

	_reset_cpu();

	reboot(LINUX_REBOOT_CMD_RESTART);

	exit(0);
}


static void 
_do_shutdown(void)
{
	int i;

	ut_mdelay(200);

	reboot(RB_ENABLE_CAD);

	printf("\r\n\r\nThe system is going down NOW !!\r\n");

	/* send the TERM and KILL signal */
	kill(-1, SIGTERM);
	ut_mdelay(2000);
	kill(-1, SIGKILL);

	for (i = 0; init_apps[i].level != 0; i++)
		init_apps[i].pid = 0;
	sync();

	ut_mdelay(2000);

	printf("\r\nThe system is halted.\r\n");

	pause();
}


static void 
_sig_exit(int signo)
{
	printf("pid %d recv signo %d\n", getpid(), signo);
	exit(0);
}


static void 
_sig_safe(int signo)
{
	init_nsigs = 1;
	init_sigs[signo] = 1;
	
	signal(signo, _sig_safe);
}


static void 
_sig_alarm(int signo)
{
	_run_level(INIT_RESPAWN);
	_run_level(INIT_RESPAWN2);
}


static void 
_sig_respawn2(int signo, siginfo_t *info, void *arg)
{


}

static void
_sig_update(int signo, siginfo_t *info, void *arg)
{
	_stop_level(INIT_RESPAWN);
	_stop_level(INIT_RESPAWN2);
	_run_level(INIT_UPDATE);
	init_sigs[SIGCONT] = 0;
}


static void 
_sig_upgrade(int signo, siginfo_t *info, void *arg)
{
	_stop_level(INIT_RESPAWN);
	_stop_level(INIT_RESPAWN2);
	_run_level(INIT_UPGRADE);
	_do_reboot();
}





/**
 *	Check the signal and run the action.
 *
 *	No return.
 */
static void 
_sig_check(void)
{
	if (init_nsigs < 1)
		return;

	/* upgrade image signals */
	if (init_sigs[SIGINT]) {

	}

	/* update signature signals */
	if (init_sigs[SIGCONT]) {
		
	}

	/* the reboot signal */
	if (init_sigs[SIGTERM]) {
		_do_reboot();
	}

	/* the halt signal */
	if (init_sigs[SIGUSR1]) {
		_do_shutdown();
	}

	init_nsigs = 0;
}

static int 
_sig_init(void)
{
	struct sigaction sa;
	
	/* set ignore signal */

	/* set exit signal */
	signal(SIGILL, _sig_exit);
	signal(SIGFPE, _sig_exit);
	signal(SIGSEGV, _sig_exit);

	/* set safe signal */
	signal(SIGUSR1, _sig_safe);	/* shutdown */
	signal(SIGTERM, _sig_safe);	/* reboot */
	signal(SIGALRM, _sig_alarm);	/* alarm */

	/* set realtime signal: respawn2 */
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = _sig_respawn2;
	sa.sa_flags = SA_SIGINFO;
	if (sigaction(SIGTRAP, &sa, NULL)) {
		printf("set SIGTRAP failed: %s\n", strerror(errno));
		return -1;
	}

	/* set realtime signal: update */
	sa.sa_sigaction = _sig_update;
	sa.sa_flags = SA_SIGINFO;
	if (sigaction(SIGUSR2, &sa, NULL)) {
		printf("set SIGKILL failed: %s\n", strerror(errno));
		return -1;
	}
	
	/* set realtime signal: upgrade */
	sa.sa_sigaction = _sig_upgrade;
	sa.sa_flags = SA_SIGINFO;
	if (sigaction(SIGINT, &sa, NULL)) {
		printf("set SIGINT failed: %s\n", strerror(errno));
		return -1;
	}



	return 0;
}




/**
 *	Check the exit program.
 *
 *	Return 0 if success.
 */
static int 
_check_exit(void)
{
	int i;
	pid_t wpid;
	int st = 0;
	int nretries = 0;

	wpid = wait(&st);

	if (wpid <= 0)
		return 0;

	nretries = sizeof(init_apps)/sizeof(init_entry_t);
	for (i = 0; i < nretries; i++) {

		if (init_apps[i].pid != wpid)
			continue;
		
		printf("process %d exited(%d)\n", wpid, st);
		
		if (init_apps[i].level == INIT_RESPAWN) {
			init_apps[i].pid = 0;
		}

		if (init_apps[i].level == INIT_RESPAWN2 &&
		    init_apps[i].status == 1) {
			init_apps[i].pid = 0;
		}
	}

	return 0;
}


/**
 *	The init main function.
 *
 *	Never return until the system is reboot, halt.
 */
int 
main(int argc, char **argv)
{
//	char buf[1024] = {0};

	ut_set_console("/dev/tty8", 0);

	ioctl(0, TIOCNOTTY, 0);

//	ctermid(buf);
//	printf("terminal is %s\n", buf);

	_sig_init();

	printf("\n\n\nSystem is started!!!\n");

	if (getpid() != 1) {
		printf("init pid(%d) is not 1\n", getpid());
	}

	/* disable CTRL-ALT-DEL as reboot key */
	reboot(RB_DISABLE_CAD);

	/* change the home directory */
	chdir("/");

	setsid();

	/* run SYSINIT entries */
	_run_level(INIT_SYSINIT);

	ut_mdelay(2000);

	/* the main loop */
	while (1) {

		_sig_check();
		
		_run_level(INIT_RESPAWN);
		
		_run_level(INIT_RESPAWN2);

		_check_exit();

		sleep(5);

		printf("run in init loop\n");
	} /* end of main loop */

	/* Cannot be here, reboot the system */
	_do_reboot();
	return 0;
}


