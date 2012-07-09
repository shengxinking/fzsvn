/*
 *    test file descript allocate rules, it always used lowest
 *    unused descriptor
 *
 *    write by Forrest.zhang
 */

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int main(void)
{
	int fd1, fd2;

	fd1 = open("test1.txt", O_WRONLY | O_CREAT, 0644);
	if (fd1 < 0)
		return -1;

	printf("the lowest descriptor is %d\n", fd1);

	close(STDERR_FILENO);
  
	fd2 = open("test2.txt", O_WRONLY | O_CREAT, 0644);
	if (fd2 < 0)
		return -1;

	printf("after close stdout, the lowest descriptor is %d\n", fd2);

	close(fd2);

	return 0;
}
