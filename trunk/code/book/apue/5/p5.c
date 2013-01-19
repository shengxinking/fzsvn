/*
 *
 */
#include <stdio.h>

int main(int argc, char** argv)
{
    if (argc != 3) {
	printf("usage: %s <directory> <prefix>\n", argv[0]);
	return 1;
    }

    printf("%s\n", tempnam(argv[1][0] != ' ' ? argv[1] : NULL,
			   argv[2][0] != ' ' ? argv[2] : NULL));
    return 0;
}
