/**
 *	@file	dbg_stack.c
 *
 *	@brief	Print the stack of function calls.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2012-08-06
 */


#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <execinfo.h>

#define	__USE_GNU
#include <link.h>
#include <dlfcn.h>
#include <signal.h>
#include <signal.h>
#include <ucontext.h>

#include "debug.h"

#define DBG_PRINT       _dbg_print

#define	DBG_MAX_FRAME	100		/* max frame stack */
#define	DBG_MAX_NAME	128		/* max symbol name length */
#define	DBG_MAX_SO	100		/* max dynamic library in process */

typedef void (*sighandler_t)(int);	/* signal handler function */

typedef struct _dbg_soaddr {
        char            name[DBG_MAX_NAME]; /* dynamic library name */
        unsigned long   address;        /* dynamic library start address */
} _dbg_soaddr_t;

static _dbg_soaddr_t    _dbg_soaddrs[DBG_MAX_SO];/* all dynamic library array*/
static int              _dbg_nsoaddr = 0;	/* number of dynamic library */
static int              _dbg_pid = 0;		/* program pid */
static char             _dbg_pname[DBG_MAX_NAME];/* program name */
static sighandler_t     _dbg_handler;		/* program signal handler */


/**
 *      The builtin print function for dbg_segfault.
 *
 *      Return print bytes if success, -1 on error.
 */
static int
_dbg_print(const char *fmt, ...)
{       
        va_list ap;
        int n;
        
        va_start(ap, fmt);
        n = vprintf(fmt, ap);
        va_end(ap);

        printf("\n");

        return (n + 1);
}

/**
 *      Get the dynamic library start address and name.
 *
 *      Save them in @_dbg_soaddrs
 */
static int
_dbg_dlphdr_callback(struct dl_phdr_info *di, size_t size, void *data)
{       
        _dbg_soaddr_t *addr;

        if (_dbg_nsoaddr >= sizeof(_dbg_soaddrs)/sizeof(_dbg_soaddrs[0]))
                return 0;

        if (di && di->dlpi_name[0] != '\0') {
                addr = &_dbg_soaddrs[_dbg_nsoaddr];
                strncpy(addr->name, di->dlpi_name, DBG_MAX_NAME - 1);
                addr->address = di->dlpi_addr;
                _dbg_nsoaddr++;
        }
        
        return 0;
}

/**
 *      Check a address is in dynamic library.
 *
 *      Return 1 if in dynamic library, 0 if not.
 */
static int
_dbg_get_soaddr(const char *soname, unsigned long *addr)
{       
        int i;

        for (i = 0; i < _dbg_nsoaddr; i++) {
                if (strncmp(soname, _dbg_soaddrs[i].name, DBG_MAX_NAME) == 0) {
                        *addr = _dbg_soaddrs[i].address;
                        return 1;
                }
        }
        
        return 0;
}

/**
 *	Dump the register content.
 *
 *	No return.
 */
static void
_dbg_dumpregs(void *ucontext)
{       
        ucontext_t *uc = ucontext;
        mcontext_t *mc;

        if (!uc)
                return;

        mc = &uc->uc_mcontext;

#ifdef  __i386__        /* i386 registers */

        DBG_PRINT("<%.6d> EAX: %.8lx   EBX: %.8lx   ECX: %.8lx    EDX: %.8lx",
                   _dbg_pid,
                   mc->gregs[REG_EAX],
                   mc->gregs[REG_EBX],
                   mc->gregs[REG_ECX],
                   mc->gregs[REG_EDX]);

        DBG_PRINT("<%.6d> ESI: %.8lx   EDI: %.8lx   EBP: %.8lx    ESP: %.8lx",
                   _dbg_pid,
                   mc->gregs[REG_ESI],
                   mc->gregs[REG_EDI],
                   mc->gregs[REG_EBP],
                   mc->gregs[REG_ESP]);

        DBG_PRINT("<%.6d> EIP: %.8lx   EFLAGS: %.8lx",
                   _dbg_pid,
                   mc->gregs[REG_EIP],
                   mc->gregs[REG_EFL]);


        DBG_PRINT("<%.6d> CS: %.4lx   GS: %.4lx   FS: %.4lx ",
                   _dbg_pid,
                   mc->gregs[REG_CS],
                   mc->gregs[REG_GS],
                   mc->gregs[REG_FS]);

        DBG_PRINT("<%.6d> Trap: %.8lx   Error: %.8lx",
                   _dbg_pid,
                   mc->gregs[REG_TRAPNO],
                   mc->gregs[REG_ERR]);

        DBG_PRINT("<%.6d> CR2: %.8lx",
                   _dbg_pid,
                   mc->gregs[REG_CR2]);#else   /* x86_64 registers */

#else

        DBG_PRINT("<%.6d> RAX     %.16lx", _dbg_pid, mc->gregs[REG_RAX]);
        DBG_PRINT("<%.6d> RBX     %.16lx", _dbg_pid, mc->gregs[REG_RBX]);
        DBG_PRINT("<%.6d> RCX     %.16lx", _dbg_pid, mc->gregs[REG_RCX]);
        DBG_PRINT("<%.6d> RDX     %.16lx", _dbg_pid, mc->gregs[REG_RDX]);

        DBG_PRINT("<%.6d> RSI     %.16lx", _dbg_pid, mc->gregs[REG_RSI]);
        DBG_PRINT("<%.6d> RDI     %.16lx", _dbg_pid, mc->gregs[REG_RDI]);
        DBG_PRINT("<%.6d> RBP     %.16lx", _dbg_pid, mc->gregs[REG_RBP]);
        DBG_PRINT("<%.6d> RSP     %.16lx", _dbg_pid, mc->gregs[REG_RSP]);

        DBG_PRINT("<%.6d> RIP     %.16lx", _dbg_pid, mc->gregs[REG_RIP]);
        DBG_PRINT("<%.6d> EFLAGS  %.16lx", _dbg_pid, mc->gregs[REG_EFL]);

        DBG_PRINT("<%.6d> CS      %.4lx", _dbg_pid,
                  (unsigned long)(mc->gregs[REG_CSGSFS] << 32 & 0xffff));
        DBG_PRINT("<%.6d> GS      %.4lx", _dbg_pid,
                  (unsigned long)(mc->gregs[REG_CSGSFS] << 16 & 0xffff));
        DBG_PRINT("<%.6d> FS      %.4lx", _dbg_pid,
                  (unsigned long)(mc->gregs[REG_CSGSFS] & 0xffff));

        DBG_PRINT("<%.6d> Trap    %.16lx", _dbg_pid, mc->gregs[REG_TRAPNO]);
        DBG_PRINT("<%.6d> Error   %.16lx", _dbg_pid, mc->gregs[REG_ERR]);
        DBG_PRINT("<%.6d> Oldmask %.16lx", _dbg_pid, mc->gregs[REG_OLDMASK]);
        DBG_PRINT("<%.6d> CR2     %.16lx", _dbg_pid, mc->gregs[REG_CR2]);

#endif

}

/**
 *	Dump the frame information, include function name,
 *	function address. etc.
 *
 *	No return.
 */
static void
_dbg_dumpsym(void *addr)
{       
        char    symname[DBG_MAX_NAME] = {0};
        unsigned long soaddr;
        const char *format;
        unsigned long offset = 0;
        Dl_info dl;
        int ret;

        if (!addr)
                return;

        ret = dladdr(addr, &dl);

        if (!ret) {
                return;
        }

        ret = _dbg_get_soaddr(dl.dli_fname, &soaddr);
        if (ret) {
                offset = (unsigned long)(addr - soaddr);
        }

        if (dl.dli_sname == NULL) {
                snprintf(symname, DBG_MAX_NAME - 1, "==> %s (+0x%.16lx) ",
                         dl.dli_fname, offset);
        }
        else {
                if (addr >= dl.dli_saddr) {
                        format = "=> %s (%s+0x%.8lx)";
                        offset = addr - dl.dli_saddr;
                }
                else {
                        format = "=> %s (%s-0x%.8lx)";
                        offset = dl.dli_saddr - addr;
                }

                snprintf(symname, DBG_MAX_NAME - 1, format,
                         dl.dli_fname, dl.dli_sname, offset);
        }

        DBG_PRINT("<%.6d> [0x%.16lx] %s", _dbg_pid, (unsigned long)addr, symname);
}

/**
 *	Signal hander for SEGFAULT, SIGILL, SIGABRT, SIGFPE.
 *
 *	It'll print the register/frame stack information.
 */
void 
_dbg_sighandler(int signo, siginfo_t *si, void *ucontext)
{
	void * frames[DBG_MAX_FRAME];
	int n, i;
	struct sigaction sa;

	/* reset the signal */
	memset(&sa, 0, sizeof(sa));
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        if (sigaction(signo, &sa, NULL)) {
                DBG_PRINT("[%s:%d] sigaction failed\n", __FILE__, __LINE__);
        }
	
	if (_dbg_handler)
		_dbg_handler(signo);

	DBG_PRINT("<%.6d> application %s", _dbg_pid, _dbg_pname);
        DBG_PRINT("<%.6d> *** signal %s received ***", 
		  _dbg_pid, strsignal(signo));

        _dbg_dumpregs(ucontext);

        /* get dynamic library name and start address */
        dl_iterate_phdr(_dbg_dlphdr_callback, NULL);

	n = backtrace(frames, DBG_MAX_FRAME);
	if (n < 2)
		return;

	/* skip 1st, 2nd stack frame, it's signal handler frame */
	for (i = 2; i < n ; ++i) {
		_dbg_dumpsym(frames[i]);
	}

	return;
}


void 
dbg_backtrace(void)
{
	void *frames[DBG_MAX_FRAME];
	int n;
	int i;
	
	/* get dynamic library name and start address */
        dl_iterate_phdr(_dbg_dlphdr_callback, NULL);

	n = backtrace(frames, DBG_MAX_FRAME);
	if (n < 2)
		return;

	for (i = 1; i < n; i++)
		_dbg_dumpsym(frames[i]);
}


int 
dbg_segfault(const char *pname, sighandler_t handler)
{
	struct sigaction sa;
	
	/* set some static variable */
	_dbg_pid = getpid();
	if (pname) 
		strncpy(_dbg_pname, pname, DBG_MAX_NAME - 1);
	else 
		strncpy(_dbg_pname, "noname", DBG_MAX_NAME - 1);
	_dbg_handler = handler;

	/* set sigaction structure */
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = _dbg_sighandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_SIGINFO;

	/* set segfault signal handler */
	if (sigaction(SIGSEGV, &sa, NULL)) {
		printf("set signal handler for %d (%s) failed\n",
		       SIGSEGV, strsignal(SIGSEGV));		
		return -1;
	}

	/* set SIGILL signal handler */
	if (sigaction(SIGILL, &sa, NULL)) {
		printf("set signal handler for %d (%s) failed\n",
		       SIGILL, strsignal(SIGILL));
		return -1;
	}
	
	/* set SIGABRT signal handler */
	if (sigaction(SIGABRT, &sa, NULL)) {
		printf("set signal handler for %d (%s) failed\n",
		       SIGABRT, strsignal(SIGABRT));
                return -1;
        }
        
	/* set SIGFPE signal handler */
        if (sigaction(SIGFPE, &sa, NULL)) {
		printf("set signal handler for %d (%s) failed\n",
		       SIGFPE, strsignal(SIGFPE));
                return -1;
        }

	return 0;
}
