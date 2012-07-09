#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

extern int ssearch(void*, size_t, size_t, const void*, int (*cmp)(const void*, const void*));

int _strcmp(const void* str1, const void* str2)
{
    char*	    p1 = *(char**)str1;
    char*	    p2 = *(char**)str2;

    return(strcmp(p1, p2));
}

int main(int argc, char* argv[])
{
    char*	arr[10] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"};
    char*	key;
    
    if (argc != 2) {
	printf("usage: %s <alpha>\n", argv[0]);
	exit(0);
    }

    key = argv[1];

    printf("%s at position: %d\n", argv[1], ssearch(arr, 10, sizeof(char*), &key, _strcmp) + 1);
    
    return 0;
}
