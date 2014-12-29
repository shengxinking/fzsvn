/**
 *	@file
 *
 *	@brief
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_INET_INFO_H
#define FZ_INET_INFO_H

typedef inet_frag {
	ip_addr_t	sip;
	ip_addr_t	dip;
	u_int16_t	id;	/* id for defrag. */
	u_int16_t	fragoff;
} inet_frag_t; 

extern inet_ctx_t * 
inet_alloc_ctx();

extern int 
inet_free_ctx();


#endif /* end of FZ_INET_INFO_H */


