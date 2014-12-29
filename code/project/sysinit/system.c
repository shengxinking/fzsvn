/**
 *	@file	system
 *	
 *	@abbrev	The system init function.
 *
 *	@date	2010-08-30
 */

#include "util.h"

int 
sys_insmod(void)
{	
	/* we need use the newly igb driver for Intel 82575X NIC*/
	ut_runcmd("/bin/busybox insmod /modules/admin_socket.ko");

	ut_runcmd("/bin/busybox insmod /modules/miglog.ko");

#if (defined CONFIG_SYSTEM_FWB_1000B) 
	ut_runcmd("/bin/busybox insmod /modules/cp6.ko");
#endif

#if (defined CONFIG_SYSTEM_FWB_1000C) || (defined CONFIG_SYSTEM_FWB_3000C)
	ut_runcmd("/bin/busybox insmod /modules/cp7vpn.ko");
#endif

	return 0;
}


int 
sys_init_cmdb(void)
{

	return 0;
}


int 
sys_update_fname(void)
{
	return 0;
}


int
sys_extract_files(void)
{
	return 0;
}



