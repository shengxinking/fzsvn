/*
 *
 */

int main(void)
{
    printf("pid = %d, gid = %d\n", getpid(), getpgid(0));
    while (1)
	;

    exit(0);
}
