/**
 *	@file	perm.h
 *	
 *	@brief	the CLI object authority check
 *
 *	@author	Forrest.zhang
 */

#ifndef CLI_PERM_H
#define CLI_PERM_H


/**
 *	CLI object perm
 */
#define CLI_GRP_READ	(0x01)		/* group read */
#define CLI_GRP_WRITE	(0x02)		/* group write */
#define CLI_GRP_RDWR	(0x03)		/* group read/write */
#define CLI_OTH_READ	(0x04)		/* other read */
#define CLI_OTH_WRITE	(0x08)		/* other write */
#define CLI_OTH_RDWR	(0x0c)		/* other read/write */


/**
 *	Check user have right to read the object
 *
 *	Return 1 if user have right, 0 not.
 */
extern int 
cli_perm_rchk(uint8 user, uint8 grp, uint8 perm);


/**
 *	Check user have right to write the object
 *
 *	Return 1 if user have right, 0 not.
 */
extern int 
cli_perm_wchk(uint8 user, uint8 grp, uint8 perm);


/**
 *	Check user have right to read/write the object
 *
 *	Return 1 if user have right, 0 not.
 */
extern int 
cli_perm_rwchk(uint8 user, uint8 grp, uint8 perm);


#endif /* end of CLI_PERM_H */

