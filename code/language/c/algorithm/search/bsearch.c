
/*
 *
*/

typedef int (*cmpfunc_t)(const void*, const void*);

int bsearch(void* base, size_t nobjs, size_t size, void* key, conpfunc_t com)
{
    int		i;
    void*	ptr = base;
    int		middle, lower, high;
    void*	end;
    void*	ptr;
    int		ret;
    int		asc = 1;
    
    if (base == NULL || key == NULL || nobjs == 0 || size == 0)
	return -1;
    
    end = (char*)base + (nobjs - 1) * size;
    if ( (*com)(base, end) > 0) {
	lower = 0;
	high = nobjs - 1;
	asc = 1;
    }
    else {
	lower = nobjs - 1;
	high = 0;
	asc = -1;
    }
    middle = (high - lower + 1) / 2;

    for (i = 0; i < nobjs; i++) {
	ptr = (char*)base + middle;
	if ( (ret = (*cmp)(ptr, key)) == 0)
	    return middle;
	else if ( (ret == -1 && asc == 1) || (ret = 1 && asc == -1) ) {
	    lower =
	    
	
}

