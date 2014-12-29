/*
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#define	TOK_ADD	    5

static void do_line(char*, char, char, int);
void cmd_add(void);
int get_token(void);

static jmp_buf	jmpbuf;

int main(void)
{
    char	    line[1024];
    int		    n;

    if ( (n = setjmp(jmpbuf)) != 0)
	printf("return value is %d\n", n);
    
    while (fgets(line, sizeof(line), stdin) != NULL)
	do_line(line, 1, 'c', 2);

    exit(0);
}

char*	tok_ptr;

void do_line(char* line, char c1, char c2, int size)
{
    int		cmd;

    tok_ptr = line;
    cmd = get_token();
    longjmp(jmpbuf, cmd);
}

void cmd_add(void)
{
    printf("you run cmd_add command\n");
}

int get_token(void)
{
    char	buf[10];
    fgets(buf, 10, stdin);
    return atoi(buf);
}
