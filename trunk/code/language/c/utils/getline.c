/*
 *  this is a file that can read a line from a file
 */

#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_CHARS	1024

char	usage[]="Usage: line [-n <Line number>] <filename>\n       -h: help\n";

static int isnumber(const char*);

int main(int argc, char* argv[])
{
	int line = 1;
	char filename[MAX_CHARS] = {0};
	char buffer[MAX_CHARS];
	FILE *fp = NULL;
	int i;
	struct stat st;
	
	if ( argc < 2 ) {
		printf(usage);
		return 1;
	}
	
	if (strcmp(argv[1], "-n") == 0) {
		if (isnumber(argv[2]))
			line = atoi(argv[2]);
		else {
			printf(usage);
			return 1;
		}
		strcpy(filename, argv[3]);
	}
	else 
		strcpy(filename, argv[1]);
	if (stat(filename, &st)) {
		perror("Error: ");
		return 1;
	}
	if (!S_ISREG(st.st_mode)) {
		printf("Error: must need regular file\n");
		return 1;
	}

	fp = fopen(filename, "r");
	
	for (i = 0; i < line; ++i) {
		if (!fgets(buffer, MAX_CHARS, fp)) {
			printf("Error: out of line ranger");
			return 1;
		}
	}
	
	printf(buffer);

	return 0;
}

int isnumber(const char* s)
{
	int i;
	for (i = 0; i < strlen(s); ++i)
		if (!isdigit(s[i]))
			break;
	if (i == strlen(s))
		return 1;
	else
		return 0;
}
