/**
 *	@file	system.h
 *
 *	@brief	The system APIs for init.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_SYSTEM_H
#define FZ_SYSTEM_H

extern int 
sys_insmod(void);


extern int 
sys_init_cmdb(void);


extern int 
sys_update_fname(void);


extern int
sys_extract_files(void);



#endif /* end of FZ_SYSTEM_H  */

