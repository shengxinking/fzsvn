/**
 *	@file	perm.c
 *
 *	@brief	Check the authority of give user and group.
 *
 *	@author	Forrest.zhang
 */

#include "perm.h"
#include "object.h"

/**
 *	Check user have right to read the object
 *
 *	Return 1 if user have right, 0 not.
 */
int 
cli_perm_rchk(u_int8_t user, u_int8_t grp, u_int8_t perm)
{
	if (user == 0)
		return 1;

	if (user == grp) {
		if (perm & CLI_OBJ_GRP_READ)
			return 1;
	}
	else {
		if (perm & CLI_OBJ_OTH_READ)
			return 1;
	}

	return 0;
}

/**
 *	Check user have right to write the object
 *
 *	Return 1 if user have right, 0 not.
 */
int 
cli_perm_wchk(u_int8_t user, u_int8_t grp, u_int8_t perm)
{
	if (user == 0)
		return 1;

	if (user == grp) {
		if (perm & CLI_OBJ_GRP_WRITE)
			return 1;
	}
	else {
		if (perm & CLI_OBJ_OTH_WRITE)
			return 1;
	}

	return 0;
}

/**
 *	Check user have right to read/write the object
 *
 *	Return 1 if user have right, 0 not.
 */
int 
cli_perm_rwchk(u_int8_t user, u_int8_t grp, u_int8_t perm)
{
	if (user == 0)
		return 1;

	if (user == grp) {
		if (perm & CLI_OBJ_GRP_READ &&
		    perm & CLI_OBJ_GRP_WRITE)
			return 1;
	}
	else {
		if (perm & CLI_OBJ_OTH_READ &&
		    perm & CLI_OBJ_OTH_WRITE)
			return 1;
	}

	return 0;
}


