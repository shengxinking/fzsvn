/*
 *
 *
 *
 */

// declaration
extern char*            pc;
extern int              iarr[10];
extern int              (&iref)[10];
extern char*            pcarr[];
extern char**           ppc;
extern const int        ci;
extern const int*       pci;
extern int *const       cpi;


// double declaration
extern char*            pc;
extern int              iarr[10];
extern int              (&iref)[10];
extern char*            pcarr[];
extern char**           ppc;
extern const int        ci;
extern const int*       pci;
extern int *const       cpi;


// define
char*            pc = 0;
int              iarr[10] = {0};
int              (&iref)[10] = iarr;
char*            pcarr[] = {"hello", "ni", "hao"};
char**           ppc = &pc;
const int        ci = 111;
const int*       pci = &ci;
int              i = 100;
int *const       cpi = &i;

int
main(void)
{
    pc = "hello";


    return 0;
}
