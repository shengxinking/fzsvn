/*
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stropts.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

int main(int argc, char** argv)
{
    int			fd, i, nmods;
    struct str_list	list;

    if (argc != 2) {
	printf("usage: %s <filename>\n", argv[0]);
	exit(1);
    }

    if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	perror(NULL);
	exit(1);
    }

    if (isastream(fd) == 0) {
	printf("%s is not a stream device\n", argv[1]);
	exit(1);
    }

/*    if ( (nmods = ioctl(fd, I_LIST, (void*)0)) < 0)
	exit(1);

    printf("%d modules\n", nmods);

    list.sl_modlist = calloc(nmods, sizeof(struct str_list));

    if (ioctl(fd, I_LIST, &list) < 0)
	exit(1);

    for (i = 0; i < nmods; i++)
	printf("%s: %s\n", (i == nmods) ? "driver" : "module", list.sl_modlist++);
*/
    exit(0);
}
