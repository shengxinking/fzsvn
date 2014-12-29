/*
 *
 *
 *
 */

int 
main(void)
{
    char          str[] = "hello";

    for (char* pc = str; pc != 0; pc++)
	*pc = 'a';

    return 0;
}
