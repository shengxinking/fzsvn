/*
 *
 */

#include <unistd.h>
#include <sys/types.h>

typedef int (*cmpfunc_t)(const void*, const void*);


//int ssearch(void* base, size_t nobjs, size_t size, void* key, int (*eq)(const void*, const void*))
int ssearch(void* base, size_t nobjs, size_t size, const void* key, cmpfunc_t eq)
{
    int		i;
    void*	ptr = base;
    
    if (base == NULL || key == NULL || nobjs == 0)
	return -1;

    for (i = 0; i < nobjs; i++) {
	if ((*eq)(ptr, key) == 0)
	    return i;
	ptr += size;
    }

    return -1;
}

