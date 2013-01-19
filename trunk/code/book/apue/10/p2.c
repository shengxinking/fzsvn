/*
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>

static void sig_alrm(int signo);

int main(void)
{
    struct passwd	*ptr;

    if (signal(SIGALRM, sig_alrm) == SIG_ERR) {
	perror("signal error");
	exit(1);
    }

    alarm(1);

    while (1) {
	if ( (ptr = getpwnam("jbug")) == NULL) {
	    perror("getpwnam error");
	    exit(1);
	}
	if (strcmp(ptr->pw_name, "jbug") != 0)
	    printf("return value corrupted!, pw_name = %s\n", ptr->pw_name);
    }

    exit(0);
}

void sig_alrm(int signo)
{
    struct passwd*	ptr;

    printf("in signal handler\n");
    if ( (ptr = getpwnam("root")) == NULL) {
	perror("getpwnam error");
	exit(1);
    }/*
    if (signal(SIGALRM, sig_alrm) == SIG_ERR) {
	perror("signal error");
	exit(1);
    }*/
    printf("reset alarm\n");
    alarm(1);
    return;
}
