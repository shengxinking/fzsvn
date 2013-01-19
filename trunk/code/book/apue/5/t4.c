/*
 *
 */

#include <time.h>

int main(void)
{
    time_t		now;
    char		buf[512];
    struct tm*		tm1;
    
    now = time(NULL);
    tm1 = localtime(&now);
    strftime(buf, 512, "%c", tm1);
    printf("%s\n", buf);

    return 0;
}
    
