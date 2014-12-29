/*
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

static void sig_int(int);
static void sig_prof(int);
static void sig_alrm(int);
static void sig_vtalrm(int);

int main(void)
{
    struct itimerval	ritv, vitv, pitv;
    
    signal(SIGINT, sig_int);
    signal(SIGPROF, sig_prof);
    signal(SIGVTALRM, sig_vtalrm);
    signal(SIGALRM, sig_alrm);

    //alarm(12);

    if (getitimer(ITIMER_REAL, &ritv) < 0) {
	printf("get REAL itimer error\n");
	perror(NULL);
	exit(1);
    }

    if (getitimer(ITIMER_VIRTUAL, &vitv) < 0) {
	printf("get VIRTUAL itimer error\n");
	perror(NULL);
	exit(1);
    }

    if (getitimer(ITIMER_PROF, &pitv) < 0) {
	printf("get PROF itimer error\n");
	perror(NULL);
	exit(1);
    }

    ritv.it_interval.tv_sec = 0;
    ritv.it_interval.tv_usec = 0;
    ritv.it_value.tv_sec = 2;
    ritv.it_value.tv_usec = 0;
    
    if (setitimer(ITIMER_REAL, &ritv, NULL) < 0) {
	printf("set REAL itimer error\n");
	perror(NULL);
	exit(1);
    }
    
    vitv.it_interval.tv_sec = 3;
    vitv.it_interval.tv_usec = 0;
    vitv.it_value.tv_sec = 3;
    vitv.it_value.tv_usec = 0;
    
    if (setitimer(ITIMER_VIRTUAL, &vitv, NULL) < 0) {
	printf("set VIRTUAL itimer error\n");
	perror(NULL);
	exit(1);
    }
    
    pitv.it_interval.tv_sec = 6;
    pitv.it_interval.tv_usec = 0;
    pitv.it_value.tv_sec = 6;
    pitv.it_value.tv_usec = 0;
    
    if (setitimer(ITIMER_PROF, &pitv, NULL) < 0) {
	printf("set PROF itimer error\n");
	perror(NULL);
	exit(1);
    }
    
    while(1)
/*	pause()*/;

    exit(0);
}

void sig_int(int signo)
{
    struct itimerval	ritv, vitv, pitv;
    
    printf("get SIGINT\n");
    ritv.it_interval.tv_sec = 2;
    ritv.it_interval.tv_usec = 0;
    ritv.it_value.tv_sec = 0;
    ritv.it_value.tv_usec = 0;
    
    if (setitimer(ITIMER_REAL, &ritv, NULL) < 0) {
	printf("set REAL itimer error\n");
	perror(NULL);
	exit(1);
    }
    
    vitv.it_interval.tv_sec = 3;
    vitv.it_interval.tv_usec = 0;
    vitv.it_value.tv_sec = 0;
    vitv.it_value.tv_usec = 0;
    
    if (setitimer(ITIMER_VIRTUAL, &vitv, NULL) < 0) {
	printf("set VIRTUAL itimer error\n");
	perror(NULL);
	exit(1);
    }
    
    pitv.it_interval.tv_sec = 6;
    pitv.it_interval.tv_usec = 0;
    pitv.it_value.tv_sec = 0;
    pitv.it_value.tv_usec = 0;
    
    if (setitimer(ITIMER_PROF, &pitv, NULL) < 0) {
	printf("set PROF itimer error\n");
	perror(NULL);
	exit(1);
    }
}

void sig_vtalrm(int signo)
{
    printf("get SIGVTALRM\n");
}

void sig_prof(int signo)
{
    printf("get SIGPROF\n");
}

void sig_alrm(int signo)
{
    printf("get SIGALRM\n");
}
