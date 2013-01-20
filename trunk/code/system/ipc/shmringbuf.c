/*
 *	@file	shmringbuf.c
 *
 *	@brief	A ring buffer using BSD share memory.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2008-07-09
 */


#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "shmringbuf.h"



