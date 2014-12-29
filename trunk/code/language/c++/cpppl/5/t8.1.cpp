/*
 *
 *
 *
 */

int 
main(void)
{
    char          str[] = "hello";

    for (int  pc = 0; str[pc] != 0; pc++)
	str[pc] = 'a';

    return 0;
}
