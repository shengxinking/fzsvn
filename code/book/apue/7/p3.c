/*
 *
 */

#include <stdio.h>
#include <stdlib.h>

#define	TOK_ADD	    5

static void do_line(char*, char, char, int);
void cmd_add(void);
int get_token(void);

int main(void)
{
    char	    line[1024];

    while (fgets(line, sizeof(line), stdin) != NULL)
	do_line(line, 1, 'c', 2);

    exit(0);
}

char*	tok_ptr;

void do_line(char* line, char c1, char c2, int size)
{
    int		cmd;

    tok_ptr = line;
    while( (cmd = get_token()) > 0) {
	switch(cmd) {
	    case TOK_ADD:
		cmd_add();
		break;
	    default:
		break;
	}
    }
}

void cmd_add(void)
{
    printf("you run cmd_add command\n");
}

int get_token(void)
{
    return TOK_ADD;
}
