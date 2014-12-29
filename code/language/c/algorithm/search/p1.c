#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern int ssearch(const void*, size_t, size_t, const void*, int (*cmp)(const void*, const void*));

int intcmp(const void* i, const void* j)
{
    int		p1 = *(int*)i;
    int		p2 = *(int*)j;
    return (p1 == p2 ? 0 : 1);
}

int main(int argc, char* argv[])
{
    int		arr[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int		key;
    int		i = 1, j = 2;
    
    if (argc != 2) {
	printf("usage: %s <number>\n", argv[0]);
	exit(0);
    }

    printf("return is: %d\n", intcmp(&i, &j));

    key = atoi(argv[1]);

    printf("%d at position: %d\n", key, ssearch(arr, 10, sizeof(int), &key, intcmp));
    
    return 0;
}
