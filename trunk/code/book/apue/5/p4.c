/*
 *
 */

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char	    name[L_tmpnam], line[1024];
    FILE*	    fp;

    printf("%s\n", tmpnam(NULL));

    tmpnam(name);
    printf("%s\n", name);

    if ( (fp = tmpfile()) == NULL) {
	printf("tmpfile error\n");
	perror("");
	exit(1);
    }

    fputs("one line of output\n", fp);
    rewind(fp);
    if (fgets(line, sizeof(line), fp) == NULL) {
	printf("fgets error\n");
	perror("");
	exit(1);
    }
    fputs(line, fp);

    exit(0);
}
