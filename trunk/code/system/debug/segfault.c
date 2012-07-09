/* Catch segmentation faults and print backtrace.
   Copyright (C) 1998, 1999, 2000, 2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1998.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */


#include <ctype.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#define __USE_GNU
#include <dlfcn.h>
#include <link.h>

#include "bp-checks.h"

typedef struct
{
	char           name[50];
	unsigned long  start_addr;
} addrList_t; 

static addrList_t gsAddrList[40];
static int        gsNumAddrList = 0;

static int dl_phdr_callback (
		struct dl_phdr_info* info,
		size_t               size,
		void*                data)
{
	if (gsNumAddrList >= sizeof(gsAddrList)/sizeof(gsAddrList[0]))
	{
		return 0;
	}

	if (info && info->dlpi_name[0] != '\0')
	{
		strncpy(gsAddrList[gsNumAddrList].name, info->dlpi_name, sizeof(gsAddrList[gsNumAddrList].name));
		gsAddrList[gsNumAddrList].name[sizeof(gsAddrList[gsNumAddrList].name) - 1] = '\0';

		gsAddrList[gsNumAddrList].start_addr = info->dlpi_addr;

		gsNumAddrList++;
	}

	return 0;
}

static int getStartAddress(char *aLibrary, unsigned long* aAddress)
{
	int ic;

	for (ic = 0; ic < gsNumAddrList; ic++)
	{
		if (strcmp(aLibrary, gsAddrList[ic].name) == 0)
		{
			*aAddress = gsAddrList[ic].start_addr;
			return 0;
		}
	}

	return -1;
}

/* Get the definition of "struct layout".  */
struct layout
{
	void *__unbounded next;
	void *__unbounded return_address;
};

/* This file defines macros to access the content of the sigcontext element
   passed up by the signal handler.  */
#include "sigcontextinfo.h"

/* This is a global variable set at program start time.  It marks the
   highest used stack address.  */
extern void *__libc_stack_end;


/* This implementation assumes a stack layout that matches the defaults
   used by gcc's `__builtin_frame_address' and `__builtin_return_address'
   (FP is the frame pointer register):

   +-----------------+     +-----------------+
   FP -> | previous FP --------> | previous FP ------>...
   |                 |     |                 |
   | return address  |     | return address  |
   +-----------------+     +-----------------+

*/

/* Get some notion of the current stack.  Need not be exactly the top
   of the stack, just something somewhere in the current frame.  */
#ifndef CURRENT_STACK_FRAME
# define CURRENT_STACK_FRAME  ({ char __csf; &__csf; })
#endif

/* By default we assume that the stack grows downward.  */
#ifndef INNER_THAN
# define INNER_THAN <
#endif

/* By default assume the `next' pointer in struct layout points to the
   next struct layout.  */
#ifndef ADVANCE_STACK_FRAME
# define ADVANCE_STACK_FRAME(next) BOUNDED_1 ((struct layout *) (next))
#endif


static int   gsPid = 0;
static char* gsProg = NULL;
static char  gsFirmware[100] = { '\0' };
static void (*gsHandler)(int sig) = NULL;

void backtraceSymbols (array, size)
	void *const *array;
	int size;
{
	int     cnt;
	char    startstring[1024];
	char    locationbuffer[1024];
	Dl_info info;

	crashlog_print("<%.5d> Backtrace:", gsPid);
	for (cnt = 0; cnt < size; ++cnt)
	{
		memset(&info, 0, sizeof(info));
		locationbuffer[0] = '\0';
		startstring[0] = '\0';

		if (dladdr(array[cnt], &info)
				&& info.dli_fname && info.dli_fname[0] != '\0')
		{
			unsigned long startAddr = 0;

			if (getStartAddress((char *)info.dli_fname, &startAddr) == 0)
			{
				sprintf(
						startstring, 
						"liboffset %.8lx", 
						(unsigned long)array[cnt] - startAddr);
			}

			if (info.dli_sname == NULL)
			{
				sprintf(locationbuffer, "=> %s ", info.dli_fname);
			}
			else
			{
				char*         format = "";
				size_t        diff;

				if (array[cnt] >= (void *) info.dli_saddr)
				{
					format = "=> %s (%s+0x%.8lx)";
					diff = array[cnt] - info.dli_saddr;
				}
				else
				{
					format = "=> %s (%s-0x%.8lx)";
					diff = info.dli_saddr - array[cnt];
				}

				sprintf(locationbuffer, format, info.dli_fname, info.dli_sname, diff);
			}
		}

		crashlog_print("<%.5d> [0x%.8lx] %s %s", gsPid, (unsigned long)array[cnt], locationbuffer, startstring);
	}
}

/* We will print the register dump in this format:

EAX: XXXXXXXX   EBX: XXXXXXXX   ECX: XXXXXXXX   EDX: XXXXXXXX
ESI: XXXXXXXX   EDI: XXXXXXXX   EBP: XXXXXXXX   ESP: XXXXXXXX

EIP: XXXXXXXX   EFLAGS: XXXXXXXX

CS:  XXXX   DS: XXXX   ES: XXXX   FS: XXXX   GS: XXXX   SS: XXXX

Trap:  XXXXXXXX   Error: XXXXXXXX   OldMask: XXXXXXXX
ESP/SIGNAL: XXXXXXXX   CR2: XXXXXXXX

*/

	static void
register_dump (struct sigcontext *ctx)
{
	/* Generate the output.  */
	crashlog_print("<%.5d> Register dump:", gsPid);

#ifdef	CONFIG_32BIT_IMAGE /* 32bit image */

	crashlog_print("<%.5d> EAX: %.8lx   EBX: %.8lx   ECX: %.8lx    EDX: %.8lx",
			gsPid,
			ctx->eax,
			ctx->ebx,
			ctx->ecx,
			ctx->edx);

	crashlog_print("<%.5d> ESI: %.8lx   EDI: %.8lx   EBP: %.8lx    ESP: %.8lx",
			gsPid,
			ctx->esi,
			ctx->edi,
			ctx->ebp,
			ctx->esp);

	crashlog_print("<%.5d> EIP: %.8lx   EFLAGS: %.8lx",
			gsPid,
			ctx->eip,
			ctx->eflags);

	crashlog_print("<%.5d> CS: %.4lx   DS: %.4lx   ES: %.4lx   FS: %.4lx   "
			"GS: %.4lx   SS: %.4lx",
			gsPid,
			(unsigned long)(ctx->cs & 0xffff),
			(unsigned long)(ctx->ds & 0xffff),
			(unsigned long)(ctx->es & 0xffff),
			(unsigned long)(ctx->fs & 0xffff),
			(unsigned long)(ctx->gs & 0xffff),
			(unsigned long)(ctx->ss & 0xffff));

	crashlog_print("<%.5d> Trap: %.8lx   Error: %.8lx   OldMask: %.8lx",
			gsPid,
			ctx->trapno,
			ctx->err,
			ctx->oldmask);

	crashlog_print("<%.5d> ESP/signal: %.8lx   CR2: %.8lx",
			gsPid,
			ctx->esp_at_signal,
			ctx->cr2);

#else /* 64 bit */

	crashlog_print("<%.5d> RAX: %.8lx   RBX: %.8lx   RCX: %.8lx    RDX: %.8lx",
			gsPid,
			ctx->rax,
			ctx->rbx,
			ctx->rcx,
			ctx->rdx);

	crashlog_print("<%.5d> RSI: %.8lx   RDI: %.8lx   RBP: %.8lx    RSP: %.8lx",
			gsPid,
			ctx->rsi,
			ctx->rdi,
			ctx->rbp,
			ctx->rsp);

	crashlog_print("<%.5d> RIP: %.8lx   EFLAGS: %.8lx",
			gsPid,
			ctx->rip,
			ctx->eflags);


	crashlog_print("<%.5d> CS: %.4lx   FS: %.4lx   GS: %.4lx ",
			gsPid,
			(unsigned long)(ctx->cs & 0xffff),
			(unsigned long)(ctx->fs & 0xffff),
			(unsigned long)(ctx->gs & 0xffff));

	crashlog_print("<%.5d> Trap: %.8lx   Error: %.8lx   OldMask: %.8lx",
			gsPid,
			ctx->trapno,
			ctx->err,
			ctx->oldmask);

	crashlog_print("<%.5d> CR2: %.8lx",
			gsPid,
			ctx->cr2);

#endif

}


/* We better should not use `strerror' since it can call far too many
   other functions which might fail.  Do it here ourselves.  */
static void
write_strsignal (int signal)
{
	if (signal < 0 || signal >= _NSIG || _sys_siglist[signal] == NULL)
	{
		crashlog_print("<%.5d> *** signal %d received ***", gsPid, signal);
	}
	else
	{
		crashlog_print("<%.5d> *** signal %d (%s) received ***",
				gsPid, signal, _sys_siglist[signal]);
	}
}

/* This function is called when a segmentation fault is caught.  The system
   is in an instable state now.  This means especially that malloc() might
   not work anymore.  */
static void
catch_segfault (int signal, SIGCONTEXT ctx)
{
	struct layout *current;
	void *__unbounded top_frame;
	void **arr;
	size_t cnt;
	struct sigaction sa;

#ifdef CONFIG_32BIT_IMAGE
	void *__unbounded top_stack;
#endif

	// reset handler it case we cause a signal here.
	sa.sa_handler = SIG_DFL;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction (signal, &sa, NULL);

	if (gsFirmware[0] != '\0')
	{
		crashlog_print("<%.5d> firmware %s", gsPid, gsFirmware);
	}
	crashlog_print("<%.5d> application %s",
			gsPid, gsProg != NULL ? gsProg : "<noname>");
	write_strsignal (signal);

	register_dump (&ctx);

	top_frame = GET_FRAME (ctx);

#ifdef CONFIG_32BIT_IMAGE
	top_stack = GET_STACK (ctx);
#endif

	/* First count how many entries we'll have.  */
	cnt = 1;
	current = BOUNDED_1 ((struct layout *) top_frame);
#ifdef CONFIG_32BIT_IMAGE
	while (!((void *) current INNER_THAN top_stack
				|| !((void *) current INNER_THAN __libc_stack_end)))
#else
		while (!((void *) current INNER_THAN __libc_stack_end))
#endif
		{
			++cnt;

			current = ADVANCE_STACK_FRAME (current->next);
		}

	arr = alloca (cnt * sizeof (void *));

	/* First handle the program counter from the structure.  */
	arr[0] = GET_PC (ctx);

	current = BOUNDED_1 ((struct layout *) top_frame);
	cnt = 1;
#ifdef CONFIG_32BIT_IMAGE
	while (!((void *) current INNER_THAN top_stack
				|| !((void *) current INNER_THAN __libc_stack_end)))
#else
		while (!((void *) current INNER_THAN __libc_stack_end))
#endif
		{
			arr[cnt++] = current->return_address;

			current = ADVANCE_STACK_FRAME (current->next);
		}

	/* If the last return address was NULL, assume that it doesn't count.  */
	if (arr[cnt-1] == NULL)
	{
		cnt--;
	}

	/* Now generate nicely formatted output.  */
	backtraceSymbols(arr, cnt);

	if (gsHandler != NULL)
	{
		gsHandler(signal);
	}
	else
	{
		/* Pass on the signal (so that a core file is produced).  */
		raise (signal);
	}
}


void 
segfault_handler(char *name, void (*handler)(int sig))
{
	struct sigaction sa;

	// load start addresses for shared libraries.
	dl_iterate_phdr(dl_phdr_callback, NULL);

	gsPid = getpid();
	gsProg = strdup(name);
	gsHandler = handler;

	{

#if 0
		char *firmware;

		firmware = nCfg_get_conf_version();
		if (firmware)
		{
			strncpy(gsFirmware, firmware, sizeof(gsFirmware));
			gsFirmware[sizeof(gsFirmware)-1] = '\0';
		}
#endif
	}

	sa.sa_handler = (void *) catch_segfault;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);
#ifdef SIGBUS
	sigaction(SIGBUS, &sa, NULL);
#endif

	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
#ifdef SIGSTKFLT
	sigaction (SIGSTKFLT, &sa, NULL);
#endif
}
