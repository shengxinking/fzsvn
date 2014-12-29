/*
 *  this program is used to read partitions of PC MBR
 *
 *  write by Forrest.zhang
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "mbr.h"

#define	_MBR_DEBUG
#ifdef	_MBR_DEBUG
#define	_MBR_ERR(fmt, args...)		fprintf(stderr, "mbr:%s:%d: " fmt, \
						__FILE__, __LINE__, ##args)
#else
#define _MBR_ERR(fmt, args...)
#endif



/**
 *	Get MBR from a disk device @devname, the disk device is
 *	SATA disk, USB disk, IDE disk, so @devname is usually /dev/sdXN,
 *	/dev/hdXN etc.
 *
 *	Return 0 if success, -1 on error.
 */
int 
mbr_get(const char *devname, mbr_t *mbr)
{
	int fd;
	int n;
	int i;
	u_int8_t buf[512];
	partition_t *partition;

	if (!devname || !mbr)
		return -1;

	fd = open(devname, O_RDONLY);
	if (fd < 0) {
		_MBR_ERR("open %s error: %s\n", 
			 devname, strerror(errno));
		return -1;
	}

	n = read (fd, buf, 512);
	if (n < 512) {
		_MBR_ERR("device %s only have %d bytes\n", 
			 devname, n);
		return -1;
	}

	/* verify MBR signature */
	if (buf[510] != 0x55 || buf[511] != 0xAA) {
		_MBR_ERR("device %s MBR signature error: 0x%2x%2x\n", 
			 devname, buf[510], buf[511]);
		return -1;
	}

	memcpy(mbr->bootcode, buf, 446);
	mbr->npartitions = 0;
	for (i = 0; i < 4; i++) {
		partition = (partition_t *)(buf + 446 + i * sizeof(partition_t));
		
		memcpy(&(mbr->partitions[i]), partition, sizeof(partition_t));
		mbr->npartitions++;
	}
	
	memcpy(&(mbr->signature), buf +510, 2);

	return 0;
}

void mbr_print(const mbr_t *mbr)
{
	int i;
	partition_t *pp;

	if (!mbr)
		return;

	printf("total %d partitions:\n", mbr->npartitions);
	printf("index\tbootid\tb_head\tb_sec\tb_cyl\tsysid"
		"\te_head\te_sec\te_cyl\ts_sec\t\tn_sec\t\n\n");
	for (i = 0; i < mbr->npartitions; i++) {
		pp = (partition_t *)&(mbr->partitions[i]);
		printf("%2d\t%4x\t%4d\t%4d\t%4d\t%4x\t%4d\t%4d\t%4d\t%8d\t%8d\n", i,
		       pp->bootid,
		       pp->beg_head,
		       pp->beg_sect & 0x3f,   /* only lower 6 bit using in sector */
		       pp->beg_cyl + ((pp->beg_sect >> 6) << 8), /* 10 bits in cylinder */
		       pp->sysid,
		       pp->end_head,
		       pp->end_sect & 0x3f,
		       (pp->end_cyl) + ((pp->beg_sect >> 6) << 8),
		       pp->start_sect,
		       pp->nr_sect);
	}
}


