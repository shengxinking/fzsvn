/**
 *	@file	bonding_util.c
 *
 *	@brief	The bonding APIs implement.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2013-10-29
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include "bonding_util.h"

#define	_BOND_MASTER_FILE	"/sys/class/net/bonding_masters"
#define	_BOND_PATH_FMT		"/sys/class/net/%s/bonding/"
#define	_BOND_ERRLEN		2048
#define	_BOND_BUFLEN		2048

#define	_BOND_DEBUG

#ifdef	_BOND_DEBUG
#define	_BOND_DBG(fmt, args...)	\
	printf("[bonding]: "fmt, ##args)
#else
#define	_BOND_DBG(fmt, args...)
#endif

#ifndef	ERR
#define	ERR(fmt, args...)	\
	printf("[%s:%d]: "fmt, __FILE__, __LINE__, ##args)
#endif

#ifndef	ERR_RET
#define	ERR_RET(ret, fmt, args...)	\
({					\
 	printf("")

/* static buffer for store error message */
static char			_bond_errbuf[_BOND_ERRLEN];

/**
 *	Read string from file @file into buffer @buf. 
 *	@buf length is @len
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_bond_read_str(const char *file, char *buf, size_t len)
{
	int fd;
	ssize_t m;

	fd = open(file, O_RDONLY);
	if (fd < 0)
		_BOND_ERR_RET(-1, "open %s failed: %s\n", file, ERRSTR)

	/* read content to @buf */
	memset(buf, 0, len);
	m = read(fd, buf, len - 1);
	close(fd);
	if (m < 0) {
		snprintf(_bond_errbuf, _BOND_ERRLEN, 
			 "<%s:%d> read %s failed: %s",
			 __FILE__, __LINE__,
			 file, strerror(errno));
		return -1;
	}

	return 0;
}

/**
 *	write string @str to file @file.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_bond_write_str(const char *file, const char *str)
{
	int fd;
	size_t n;
	ssize_t m;

	fd = open(file, O_WRONLY);
	if (fd < 0) {
		snprintf(_bond_errbuf, _BOND_ERRLEN, 
			 "<%s:%d> open %s readonly failed: %s",
			 __FILE__, __LINE__,
			 file, strerror(errno));
		return -1;
	}

	/* write @str to file */
	n = strlen(str);
	m = write(fd, str, n);
	close(fd);
	if (m != n) {
		snprintf(_bond_errbuf, _BOND_ERRLEN, 
			 "<%s:%d> write(%s) %s failed: %s",
			 __FILE__, __LINE__,
			 str, file, strerror(errno));
		return -1;
	}

	return 0;
}

/**
 *	Check bonding argument @arg is valid or not.
 *
 * 	Return 0 if valid , -1 on error.
 */
static int 
_bond_check_arg(const bonding_arg_t *arg)
{
	int i;

	/* check miimon, if not setting, set to 100 as good value */
	if (arg->miimon < 0) {
		snprintf(_bond_errbuf, _BOND_ERRLEN,
			 "<%s:%d> miimon %d is invalid", 
			 __FILE__, __LINE__, arg->miimon);
		return -1;
	}

	/* check mode */
	if (arg->mode < 0 || arg->mode > BOND_MODE_ALB) {
		snprintf(_bond_errbuf, _BOND_ERRLEN,
			 "<%s:%d> mode %d is invalid", 
			 __FILE__, __LINE__, arg->mode);
		return -1;
	}

	if (arg->lacp_rate < 0 || arg->lacp_rate > 1) {
		snprintf(_bond_errbuf, _BOND_ERRLEN,
			 "<%s:%d> lacp_rate %d is invalid", 
			 __FILE__, __LINE__, arg->lacp_rate);
		return -1;
	}

	/* check hash xmit policy */
	if (arg->xmit_hash_policy < BOND_XMIT_POLICY_LAYER2 || 
	    arg->xmit_hash_policy > BOND_XMIT_POLICY_LAYER23)
	{
		snprintf(_bond_errbuf, _BOND_ERRLEN,
			 "<%s:%d> xmit_hash_policy %d is invalid", 
			 __FILE__, __LINE__, arg->xmit_hash_policy);
		return -1;
	}

	/* check slave interfaces number */
	if (arg->nslave < 0 || arg->nslave > BONDING_MAX_SLAVE)
	{
		snprintf(_bond_errbuf, _BOND_ERRLEN,
			 "<%s:%d> slave number %d is invalid", 
			 __FILE__, __LINE__, arg->nslave);
		return -1;
	}

	/* check slave interface is valid physical interface, FIXED ME */
	for (i = 0; i < arg->nslave; i++)
	{
		/* need implement API check a interface name is a 
		 * physical interface, not VLAN, Bridge, bonding, TUN etc. */
	}

	return 0;
}

/**
 *	Check 2 bonding argument @arg1 & arg2 is same or not.
 *
 *	Return 0 if is same, or else not same.
 */
static int 
_bond_cmp_arg(const bonding_arg_t *arg1, const bonding_arg_t *arg2)
{
	int i;
	int ret;

	/* compare mode */
	if (arg1->mode != arg2->mode)
		return (arg1->mode - arg2->mode);

	/* compare xmit_hash_policy */
	if (arg1->xmit_hash_policy != arg2->xmit_hash_policy)
		return (arg1->xmit_hash_policy - arg2->xmit_hash_policy);

	/* compare lacp_rate */
	if (arg1->lacp_rate != arg2->lacp_rate)
		return (arg1->lacp_rate - arg2->lacp_rate);

	/* compare miimon */
	if (arg1->miimon != arg2->miimon)
		return (arg1->miimon - arg2->miimon);

	/* compare slave number */
	if (arg1->nslave != arg2->nslave)
		return (arg1->nslave - arg2->nslave);
	
	/* compare each slave interface name */
	for (i = 0; i < arg1->nslave; i++)
	{
		ret = strcmp(arg1->slaves[i], arg2->slaves[i]);
		if (ret)
			return ret;
	}

	return 0;
}

/**
 *	Add bonding name @name to master file.
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_bond_add_master(const char *name)
{
	size_t n;
	char buf[32];

	/* check bonding name length and is not "bonding_masters" */
	n = strlen(name);
	if (n < 1 || n > IFNAMSIZ - 1 ||
	    strcmp(name, "bonding_masters") == 0)
	{
		snprintf(_bond_errbuf, _BOND_ERRLEN,
			 "<%s:%d> invalid bonding name %s\n", 
			 __FILE__, __LINE__, name);
		return -1;
	}

	/* add bonding name to master */
	snprintf(buf, sizeof(buf), "+%s", name);
	if (_bond_write_str(_BOND_MASTER_FILE, buf))
		return -1;

	return 0;
}

/**
 * 	Delete bonding from master file, not @name is NULL, delete all 
 * 	bonding interfaces.
 *
 * 	return 0 if success, -1 on error.
 */
static int 
_bond_del_master(const char *name)
{
	char buf[2048];
	char bondname[32];
	char *ptr;
	char *saveptr = NULL;

	/* get all bonding device from master */
	if (_bond_read_str(_BOND_MASTER_FILE, buf, sizeof(buf)))
		return -1;

	/* iterate all bonding device */
	ptr = strtok_r(buf, " ", &saveptr);
	while (ptr)
	{
		/* delete bonding device if @name is NULL or @name == @ptr */
		if (!name || strcmp(name, ptr) == 0) {
			snprintf(bondname, sizeof(bondname), "-%s", ptr);
			if (_bond_write_str(_BOND_MASTER_FILE, buf))
				return -1;
		}

		ptr = strtok_r(NULL, " ", &saveptr);
	}
	
	return 0;
}

/**
 *	Find bonding name in master file
 *
 *	return 1 if found, 0 not found, -1 on error.
 */
static int 
_bond_find_master(const char *name)
{
	char buf[2048];
	char *saveptr = NULL;
	char *bondname;

	/* read all bonding name from master file */
	if (_bond_read_str(_BOND_MASTER_FILE, buf, sizeof(buf)))
		return -1;

	/* check @name exist */
	bondname = strtok_r(buf, " ", &saveptr);
	while(bondname) {
		if (strcmp(bondname, name) == 0)
			return 1;

		bondname = strtok_r(NULL, " ", &saveptr);
	}

	/* not founded */
	return 0;
}

/**
 *	Get bonding device @name's mode..
 *
 * 	Return >=0 if success, -1 on error.
 */
static int 
_bond_get_mode(const char *name)
{
	char path[128];
	char file[128];
	char buf[128];

	/* get bonding mode filename in sysfs */
	snprintf(path, sizeof(path), _BOND_PATH_FMT, name);
	snprintf(file, sizeof(file), "%s/mode", path);

	if (_bond_read_str(file, buf, sizeof(buf)))
		return -1;

	return atoi(buf);
}

/**
 *	Set bonding device @name's mode as @mode.
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_bond_set_mode(const char *name, int mode)
{
	char path[128];
	char file[128];
	char buf[128];

	/* get bonding mode filename in sysfs */
	snprintf(path, sizeof(path), _BOND_PATH_FMT, name);
	snprintf(file, sizeof(file), "%s/mode", path);

	snprintf(buf, sizeof(buf), "%d", mode);

	if (_bond_write_str(file, buf))
		return -1;

	return 0;
}

/**
 *	Get bonding device @name's xmit_hash_policy
 *
 *	Return >= 0 if success, -1 on error.
 */
static int 
_bond_get_xmit_hash_policy(const char *name)
{
	char path[128];
	char file[128];
	char buf[128];

	/* get bonding xmit_hash_policy filename in sysfs */
	snprintf(path, sizeof(path), _BOND_PATH_FMT, name);
	snprintf(file, sizeof(file), "%s/xmit_hash_policy", path);

	if (_bond_read_str(file, buf, sizeof(buf)))
		return -1;

	return atoi(buf);
}

/**
 *	Set bonding device @name's xmit_hash_policy as @policy.
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_bond_set_xmit_hash_policy(const char *name, int policy)
{
	char path[128];
	char file[128];
	char buf[128];

	/* get bonding xmit_hash_policy filename in sysfs */
	snprintf(path, sizeof(path), _BOND_PATH_FMT, name);
	snprintf(file, sizeof(file), "%s/xmit_hash_policy", path);

	/* write to file */
	snprintf(buf, sizeof(buf), "%d", policy);
	if (_bond_write_str(file, buf))
		return -1;

	return 0;
}

/**
 *	Get bonding device @name's lacp_rate
 *
 *	Return >= 0 if success, -1 on error.
 */
static int 
_bond_get_lacp_rate(const char *name)
{
	char path[128];
	char file[128];
	char buf[128];

	/* get bonding lacp_rate filename in sysfs */
	snprintf(path, sizeof(path), _BOND_PATH_FMT, name);
	snprintf(file, sizeof(file), "%s/lacp_rate", path);

	if (_bond_read_str(file, buf, sizeof(buf)))
		return -1;

	return atoi(buf);
}

/**
 *	Set bonding device @name's lacp_rate as @rate.
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_bond_set_lacp_rate(const char *name, int rate)
{
	char path[128];
	char file[128];
	char buf[128];

	/* get bonding lacp_rate filename in sysfs */
	snprintf(path, sizeof(path), _BOND_PATH_FMT, name);
	snprintf(file, sizeof(file), "%s/lacp_rate", path);

	snprintf(buf, sizeof(buf), "%d", rate);

	if (_bond_write_str(file, buf))
		return -1;

	return 0;

}

/**
 *	Get bonding device @name's miimon 
 *
 *	Return >= 0 if success, -1 on error.
 */
static int 
_bond_get_miimon(const char *name)
{
	char path[128];
	char file[128];
	char buf[128];

	/* get bonding miimon filename in sysfs */
	snprintf(path, sizeof(path), _BOND_PATH_FMT, name);
	snprintf(file, sizeof(file), "%s/miimon", path);

	if (_bond_read_str(file, buf, sizeof(buf)))
		return -1;

	return atoi(buf);
}

/**
 *	Set bonding device @name's miimon as @miimon.
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_bond_set_miimon(const char *name, int miimon)
{
	char path[128];
	char file[128];
	char buf[128];

	/* get bonding miimon filename in sysfs */
	snprintf(path, sizeof(path), _BOND_PATH_FMT, name);
	snprintf(file, sizeof(file), "%s/miimon", path);

	snprintf(buf, sizeof(buf), "%d", miimon);

	if (_bond_write_str(file, buf))
		return -1;

	return 0;
}

/**
 *	Set bonding device @name's parameter as @arg.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_bond_set_arg(const char *name, const bonding_arg_t *arg)
{
	if (_bond_set_mode(name, arg->mode))
		return -1;

	if (_bond_set_xmit_hash_policy(name, arg->xmit_hash_policy))
		return -1;

	if (_bond_set_lacp_rate(name, arg->lacp_rate))
		return -1;

	if (_bond_set_miimon(name, arg->miimon))
		return -1;

	return 0;
}

/**
 *	Add interfaces into bonding device @name. the 
 *	interfaces stored in @arg.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_bond_add_slaves(const char *name, const bonding_arg_t *arg)
{
	char path[128];
	char file[128];
	char slave[128];
	int i;

	/* get bonding slave filename in sysfs */
	snprintf(path, sizeof(path), _BOND_PATH_FMT, name);
	snprintf(file, sizeof(file), "%s/slave", path);

	/* add new slave interfaces */
	for (i = 0; i < arg->nslave; i++) {
		snprintf(slave, sizeof(slave), "+%s", arg->slaves[i]);
		if (_bond_write_str(file, slave))
			return -1;
	}

	return 0;
}

/**
 *	Delete interfaces from bonding device @name. the 
 *	interfaces stored in @arg.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_bond_del_slaves(const char *name, bonding_arg_t *arg)
{
	char path[128];
	char file[128];
	char slave[128];
	int i;

	/* get bonding slave filename in sysfs */
	snprintf(path, sizeof(path), _BOND_PATH_FMT, name);
	snprintf(file, sizeof(file), "%s/slave", path);

	/* del all slave interfaces */
	for (i = 0; i < arg->nslave; i++) {
		snprintf(slave, sizeof(slave), "-%s", arg->slaves[i]);
		if (_bond_write_str(file, slave))
			return -1;
	}

	return 0;
}

const char * 
bonding_get_error(void)
{
	return _bond_errbuf;
}

int 
bonding_is_exist(const char *name)
{
	int ret;

	ret = _bond_find_master(name);

	return (ret == 1) ? 1 : 0;
}

int 
bonding_add(const char *name, const bonding_arg_t *arg)
{
	if (!name || !arg)
		return -1;

	/* check bonding is exist */
	if (_bond_find_master(name) == 0)
		return -1;

	/* check args */
	if (_bond_check_arg(arg))
		return -1;

	/* add master */
	if (_bond_add_master(name))
		return -1;

	/* set bonding arg */
	if (_bond_set_arg(name, arg))
		return -1;

	/* add all slave interfaces */
	if (_bond_add_slaves(name, arg))
		return -1;

	return 0;
}

int 
bonding_modify(const char *name, const bonding_arg_t *arg)
{
	bonding_arg_t oldarg;
	
	if (!name || !arg)
		return -1;

	/* check bonding is exist or not */
	if (_bond_find_master(name) != 1)
		return -1;

	/* check bond arg */
	if (_bond_check_arg(arg))
		return -1;

	/* get old bonding arg */
	if (bonding_get_arg(name, &oldarg))
		return -1;

	/* check @args is same with @oldarg */
	if (_bond_cmp_arg(arg, &oldarg) == 0)
		return 0;

	/* need remove old interface before change parameter ?? */
	if (_bond_del_slaves(name, &oldarg))
		return -1;

	/* mode changed, need commit it */
	if (arg->mode != oldarg.mode && 
	    _bond_set_mode(name, arg->mode))
		return -1;
		
	/* xmit_hash_policy changed, need commit it */
	if (arg->xmit_hash_policy != oldarg.xmit_hash_policy && 
	    _bond_set_xmit_hash_policy(name, arg->xmit_hash_policy))
		return -1;
	
	/* lacp_rate changed, need commit it */
	if (arg->lacp_rate != oldarg.lacp_rate && 
	    _bond_set_lacp_rate(name, arg->lacp_rate))
		return -1;

	/* miimon changed, need commit it */
	if (arg->miimon != oldarg.miimon && 
	    _bond_set_miimon(name, arg->miimon))
		return -1;
		
	/* need add all new slave interfaces */
	if (_bond_add_slaves(name, arg))
		return -1;

	return 0;
}

int 
bonding_del(const char *name)
{
	if (!name)
		return -1;

	/* bonding not exist */
	if (_bond_find_master(name) != 1)
		return -1;

	if (_bond_del_master(name))
		return -1;

	return 0;
}

int 
bonding_get_arg(const char *name, bonding_arg_t *arg)
{
	char buf[2048];
	char path[128];
	char file[128];
	char *slave;
	char *saveptr = NULL;
	int nslave = 0;

	if (!name || !arg)
		return -1;

	memset(arg, 0, sizeof(bonding_arg_t));

	/* get mode */
	arg->mode = _bond_get_mode(name);
	if (arg->mode < 0)
		return -1;

	/* get xmit_hash_policy */
	arg->xmit_hash_policy = _bond_get_xmit_hash_policy(name);
	if (arg->xmit_hash_policy < 0)
		return -1;

	/* get lacp_rate */
	arg->lacp_rate = _bond_get_lacp_rate(name);
	if (arg->lacp_rate < 0)
		return -1;

	/* get miimon */
	arg->miimon = _bond_get_miimon(name);
	if (arg->miimon < 0)
		return -1;

	/* get bonding slave filename in sysfs */
	snprintf(path, sizeof(path), _BOND_PATH_FMT, name);
	snprintf(file, sizeof(file), "%s/slave", path);

	/* read all slave interfaces */
	if (_bond_read_str(file, buf, sizeof(buf)))
		return -1;

	/* save all slave interfaces in @arg */
	slave = strtok_r(buf, " ", &saveptr);
	while(slave) 
	{
		strncpy(arg->slaves[nslave], slave, IFNAMSIZ);
		nslave++;

		slave = strtok_r(NULL, " ", &saveptr);
	}
	arg->nslave = nslave;

	return 0;
}

int 
bonding_get_number(void)
{
	char buf[2048];
	char *saveptr = NULL;
	char *bondname;
	int n = 0;

	/* read all bonding name from master file */
	if (_bond_read_str(_BOND_MASTER_FILE, buf, sizeof(buf)))
		return -1;

	/* check @name exist */
	bondname = strtok_r(buf, " ", &saveptr);
	while(bondname) {
		bondname = strtok_r(NULL, " ", &saveptr);
		n++;
	}

	return n;
}

char ** 
bonding_get_all(char *buf, size_t len)
{
	char bonds[2048];
	char bonds1[2048];
	char *saveptr = NULL;
	char *bondname;
	char **ptr, *start;
	size_t size;
	int n = 0;
	int i = 0;

	/* read all bonding name from master file */
	if (_bond_read_str(_BOND_MASTER_FILE, bonds, sizeof(bonds)))
		return NULL;

	/* save bonds */
	memcpy(bonds1, bonds, sizeof(bonds));

	/* get bonding count */
	bondname = strtok_r(bonds1, " ", &saveptr);
	while(bondname) {
		bondname = strtok_r(NULL, " ", &saveptr);
		n++;
	}

	/* calculate size, add last NULL pointer size and zero end */
	size = strlen(bonds) + (n + 1) * sizeof(char **) + 1;

	/* not enough size */
	if (len < size) {
		snprintf(_bond_errbuf, _BOND_ERRLEN, 
			 "<%s:%d> small buffer %lu store %lu bytes",
			 __FILE__, __LINE__, len, size);
		return NULL;
	}

	ptr = (char **)buf;
	start = buf + (n + 1) * sizeof(char **);

	/* no bonding device */
	if (n == 0) {
		ptr[0] = NULL;
		return ptr;
	}

	/* save bonding string from start */
	strcpy(start, bonds);
	bondname = strtok_r(start, " ", &saveptr);
	while (bondname) {
		ptr[i] = bondname;
		i++;
		bondname = strtok_r(NULL, " ", &saveptr);
	}
	ptr[i] = NULL;

	return ptr;
}

int 
bonding_del_all(void)
{
	if (_bond_del_master(NULL))
		return -1;

	return 0;
}


