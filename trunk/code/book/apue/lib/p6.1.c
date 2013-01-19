/*
 *
 */

#include <pwd.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>

struct passwd* my_getpwnam_r(const char* name)
{
    struct passwd*	pwd, *ptr;

    setpwent();
    while ( (ptr = getpwent()) != NULL)
	if (strcmp(pwd->pw_name, name) == 0)
	    break;

    if (pwd != NULL) {
	pwd = malloc(sizeof(struct passwd));
	memcpy(pwd, ptr, sizeof(struct passwd));
    }
    else
	pwd = NULL;

    endpwent();
    return (pwd);
}
