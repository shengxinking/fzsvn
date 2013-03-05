/**
 *	@file	dbg_segfault.c
 *
 *	@brief	the segfault handler which print the stack of process.
 *
 * 	@author	Forrest.Zhang.
 */


#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <execinfo.h>

#define	__USE_GNU	1
#include <dlfcn.h>
#include <link.h>
#include <signal.h>
#include <ucontext.h>

#include "debug.h"

#define	DBG_MAX_FRAME	100
#define	DBG_MAX_SO	100
#define	DBG_MAX_NAME	128

#define	DBG_PRINT	crashlog_print

typedef void (*sighandler_t)(int signo);

typedef struct _dbg_soaddr {
	char		name[DBG_MAX_NAME]; /* dynamic library name */
	unsigned long	address;	/* dynamic library start address */
} _dbg_soaddr_t;

static _dbg_soaddr_t	_dbg_soaddrs[DBG_MAX_SO];
static int		_dbg_nsoaddr = 0;
static int		_dbg_pid = 0;
static char		_dbg_pname[DBG_MAX_NAME];
static sighandler_t	_dbg_handler;

/**
 *	The builtin print function for dbg_segfault.
 *
 *	Return print bytes if success, -1 on error.
 */
int 
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
 *	Get the dynamic library start address and 
 *
 *	Save the address in @_dbg_soaddrs
 */
static int  
_dbg_dlphdr_callback(struct dl_phdr_info *di, size_t size, void *data)
{
	_dbg_soaddr_t *addr;

	if (_dbg_nsoaddr >= sizeof(_dbg_soaddrs)/sizeof(_dbg_soaddrs[0]))
		return 0;

	if (di && di->dlpi_name[0] != '\0') {
		addr = &_dbg_soaddrs[_dbg_nsoaddr];
		strlcpy(addr->name, di->dlpi_name, DBG_MAX_NAME - 1);
		addr->address = di->dlpi_addr;
		_dbg_nsoaddr++;
	}

	return 0;
}

static void 
_dbg_dumpreg(void *ucontext)
{
	ucontext_t *uc = ucontext;
	mcontext_t *mc;

	if (!uc)
		return;

	mc = &uc->uc_mcontext;

#ifdef	__i386__	/* i386 registers */

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
                   mc->gregs[REG_CR2]);

#else	/* x86_64 registers */
	
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
 *	Check a address is in dynamic library.
 *
 * 	Return 1 if in dynamic library, 0 if not.
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


static void 
_dbg_dumpsym(void *addr)
{
	char	symname[DBG_MAX_NAME] = {0};
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
		snprintf(symname, DBG_MAX_NAME - 1, "==> %s + 0x%.16lx) ", 
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

static void 
_dbg_sighandler(int signo, siginfo_t *si, void *ucontext)
{
	void *frames[DBG_MAX_FRAME];
	int n;
	struct sigaction sa;
	int i;

	/* reset the signal */
	sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(signo, &sa, NULL)) {
		DBG_PRINT("[%s:%d] sigaction failed\n", __FILE__, __LINE__);
	}
	
	DBG_PRINT("<%.6d> application %s", _dbg_pid, _dbg_pname);
	DBG_PRINT("<%.6d> *** signal %s received ***", _dbg_pid, strsignal(signo));

	_dbg_dumpreg(ucontext);

	/* get dynamic library name and start address */
	dl_iterate_phdr(_dbg_dlphdr_callback, NULL);	

	/* get backtrace */
	n = backtrace(frames, DBG_MAX_FRAME);
	if (n < 1)
		return;

	/* print the function name, address */
	for (i = 1; i < n; i++) {
		_dbg_dumpsym(frames[i]);
	}

	_dbg_handler(signo);

	_exit(SIGSEGV);
}


int 
dbg_segfault(const char *pname, void (*handler)(int signo))
{
	struct sigaction sa;

	/* set some variable */
	_dbg_pid = getpid();
	if (pname) {
		strlcpy(_dbg_pname, pname, DBG_MAX_NAME);
	}
	else {
		strlcpy(_dbg_pname, "noname", DBG_MAX_NAME);
	}
	_dbg_handler = handler;

	/* set signal handler */
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = _dbg_sighandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;

	if (sigaction(SIGSEGV, &sa, NULL)) {
		return -1;
	}

	if (sigaction(SIGILL, &sa, NULL)) {
		return -1;
	}

	if (sigaction(SIGABRT, &sa, NULL)) {
		return -1;
	}

	if (sigaction(SIGFPE, &sa, NULL)) {
		return -1;
	}

	return 0;
}


