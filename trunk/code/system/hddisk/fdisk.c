#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <linux/hdreg.h>	/* for HDIO_GETGEO */
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#include "cfgcommon.h"
#include "fdisk.h"
#include "util.h"
#include "sysapi.h"
//#include "cfgapi.h"
//#include "flg.h"
#include "ide.h"
#include "strlcpy.h"
#include "autoconf.h"
#include "logdisk.h"

#ifdef HAVE_HW_RAID
#include "hwpart.h"
static int partition_hwraid_logdisk (char *dev);
#endif

static const unsigned long long SWAP_SIZE = FLG_SWAP_SIZE;

static long ext2_llseek(unsigned int fd, long offset, unsigned int origin);
static void reread_partition_table(int dev);

extern char * get_log_disk_info(char **mpoint);
extern int probe_logdisk();
int lock_crash_create(void);

static void write_part_table_flag(char *b)
{
    b[510] = 0x55;
    b[511] = 0xaa;
}

static int logdiskfg = 0;

static void store4_little_endian(unsigned char *cp, unsigned int val)
{
    cp[0] = (val & 0xff);
    cp[1] = ((val >> 8) & 0xff);
    cp[2] = ((val >> 16) & 0xff);
    cp[3] = ((val >> 24) & 0xff);
}

static void set_start_sect(struct partition *p, unsigned int start_sect)
{
    store4_little_endian(p->start4, start_sect);
}

static void set_nr_sects(struct partition *p, unsigned int nr_sects)
{
    store4_little_endian(p->size4, nr_sects);
}

char MBRbuffer[MAX_SECTOR_SIZE];
char *disk_device;		/* must be specified */
int fd, partitions = 4;		/* maximum partition + 1 */

unsigned long long cylinders;
unsigned long long sectors;
uint heads, sector_size = DEFAULT_SECTOR_SIZE, sector_offset = 1;

#define non_fatal() {                           \
	message(" ERROR ");                     \
}

// mdelay - delay w/ millisecond granularity
// Use mdelay instead of sleep in case we want to use alarm.
void mdelay(int len)
{
    struct timespec sleep_tm;

    sleep_tm.tv_sec = len / 1000;
    sleep_tm.tv_nsec = (len - sleep_tm.tv_sec * 1000) * 1000000;
    while (nanosleep(&sleep_tm, &sleep_tm) == -1 && errno == EINTR) ;
}

static void seek_sector(int fd, uint secno)
{
    long offset = (long) secno * sector_size;
    if (ext2_llseek(fd, offset, SEEK_SET) == (long) -1)
	non_fatal();
}

static void write_sector(int fd, uint secno, char *buf)
{
    seek_sector(fd, secno);
    if (write(fd, buf, sector_size) != sector_size)
	non_fatal();
}

static void set_partition(int i, int doext, uint start, uint stop, int sysid)
{
    struct partition *p;
    uint offset;

    p = ptes[i].part_table;
    offset = ptes[i].offset;
    p->boot_ind = 0;
    p->sys_ind = sysid;
    set_start_sect(p, start - offset);
    set_nr_sects(p, stop - start + 1);

    if ((start / (sectors * heads) > 1023))
	start = heads * sectors * 1024 - 1;
    set_hsc(p->head, p->sector, p->cyl, start);

    if ((stop / (sectors * heads) > 1023))
	stop = heads * sectors * 1024 - 1;
    set_hsc(p->end_head, p->end_sector, p->end_cyl, stop);

    ptes[i].changed = 1;
}

static int linux_version_code(void)
{
    static int kernel_version = 0;
    struct utsname my_utsname;
    int p, q, r;

    if (!kernel_version && uname(&my_utsname) == 0) {
	p = atoi(strtok(my_utsname.release, "."));
	q = atoi(strtok(NULL, "."));
	r = atoi(strtok(NULL, "."));
	kernel_version = MAKE_VERSION(p, q, r);
    }
    return kernel_version;
}

static void get_sectorsize(int fd)
{
    if (linux_version_code() >= MAKE_VERSION(2, 3, 3))
    {
        int arg;
        if (ioctl(fd, BLKSSZGET, &arg) == 0)
        {
            sector_size = arg;
        }else
        {
            //message("Error getting sector size %s %d \n", __FUNCTION__, __LINE__);
        }
        if (sector_size != DEFAULT_SECTOR_SIZE)
        {
            message(_("Note: sector size is %d (not %d)\n"), sector_size, DEFAULT_SECTOR_SIZE);
        }
    }
}

long ext2_llseek(unsigned int fd, long offset, unsigned int origin)
{
    if ((sizeof(off_t) < sizeof(long)) && (offset >= ((long) 1 << ((sizeof(off_t) * 8) - 1)))) {
	errno = EINVAL;
	return -1;
    }
    return (long) lseek(fd, (off_t) offset, origin);
}

void print_hd_geometry(const struct hd_geometry *geo)
{
    message("geo->Hds,Secs,Cyls,Start = %u  %u  %u  %lu\n",
	    geo->heads, geo->sectors, geo->cylinders, geo->start);
}

void print_hd_geometry_big(const struct hd_big_geometry *geo)
{
    message("geo->Hds,Secs,Cyls,Start = %u  %u  %u  %lu\n",
	    geo->heads, geo->sectors, geo->cylinders, geo->start);
}

void print_fdisk_geo(void)
{
    message("\theads = %u, sectors = %llu, cylinders = %llu\n\tsector_size = %u, sector_offset = %u\n",
	    heads, sectors, cylinders, sector_size, sector_offset);
}

static void get_geometry(int fd)
{
    int sec_fac;
    unsigned long long longlongsectors;
    unsigned long longsectors;
    struct hd_geometry geometry;
    int res1, res2;

    get_sectorsize(fd);
    //message("%s:%d:%s: sector_size = %u\n", __FILE__, __LINE__, __FUNCTION__, sector_size);
    sec_fac = sector_size / 512;

    res1 = ioctl(fd, BLKGETSIZE, &longsectors);   // > 2TB, this call will fail.
    if(res1 != 0){
        res1 = ioctl(fd, BLKGETSIZE64, &longlongsectors);
        longlongsectors = longlongsectors / 512;
    }else
    {
        longlongsectors = longsectors;
    }
    
    //message("%s:%d:%s: longsectors=%lld\n", __FILE__, __LINE__, __FUNCTION__, longlongsectors);
    memset(&geometry, 0, sizeof(geometry));
    res2 = ioctl(fd, HDIO_GETGEO, &geometry);
    //message("%s:%d:%s: res1 = %d\tres2 = %d\n", __FILE__, __LINE__, __FUNCTION__, res1, res2);
//    print_hd_geometry(& geometry);

    heads = cylinders = sectors = 0;
    sector_offset = 1;
    if (res2 == 0) {
        heads = geometry.heads;
        sectors = geometry.sectors;
        if (heads * sectors == 0)
            res2 = -1;
        else
            sector_offset = sectors;
    }

    if (res1 == 0 && res2 == 0) {	/* normal case */
        cylinders = longlongsectors / (heads * sectors);
        cylinders /= sec_fac;	/* do not round up */
    } else if (res1 == 0) 	/* size but no geometry */
    {
        heads = cylinders = 1;
        sectors = longlongsectors / sec_fac;
    }

    //print_fdisk_geo();
}

static int get_boot(enum action what)
{
    int i;

    char real_disk_device[128];
    char dr = -1;
    int no = -1;

    /* Mantis 8653 : if the disk_device matches * /dev/hd[a-z][0-9] 
     * then we will remove the trailing number */
    strncpy(real_disk_device, disk_device, 128);
    if (2 == sscanf(real_disk_device, "/dev/hd%c%d", &dr, &no)) {
	if (dr >= 'a' && dr <= 'z' && no >= 0 && no <= 3) {
	    real_disk_device[8] = 0;
	    //message("fdisk.c: using %s\n", real_disk_device);
	}
    }

    partitions = 4;
    //message("%s:%d:%s  disk_device='%s'\n", __FILE__, __LINE__, __FUNCTION__, real_disk_device);
    if ((fd = open(real_disk_device, O_RDWR)) < 0) {
	if ((fd = open(real_disk_device, O_RDONLY)) < 0) {
	    if (what == try_only)
		return 1;
	    non_fatal();
	} else
	    printf(_("You will not be able to write the partition table.\n"));
    }

    get_geometry(fd);
    for (i = 0; i < 4; i++) {
	struct pte *pe = &ptes[i];
//message("%s:%d:%s  \n", __FILE__, __LINE__, __FUNCTION__);
	pe->part_table = pt_offset(MBRbuffer, i);
	pe->ext_pointer = NULL;
	pe->offset = 0;
	pe->sectorbuffer = MBRbuffer;
	pe->changed = (what == create_empty_dos);
    }
    return 0;
}

static void add_partitions()
{
#if !defined(CONFIG_FLG_FMG_COMB)
    unsigned long long swap_start;
    unsigned long long log_start, stop = 0;
    unsigned long long limit;
    
    log_start = sector_offset;
    limit = heads * sectors;
    limit = limit * cylinders;
    limit -= 1;
    if(limit > 0xffffffff)
    {
        message("The number of sectors '%llu' is larger than the max.\nSetting it to max '%lu' sectors.\n", limit, (unsigned long)0xffffffff);
        limit = 0xffffffff;
    }
    stop = limit;
    swap_start   = (stop - (SWAP_SIZE/sector_size));

#ifdef HAVE_SWAP_ON_DISK
    set_partition(0, 0, log_start, swap_start - 1, LINUX_NATIVE);
    set_partition(1, 0, swap_start, stop, LINUX_SWAP);
#else
    set_partition(0, 0, log_start, stop, LINUX_NATIVE);
#endif

#else
//two partitions /sda1 and /sda2 for FortiAnalyzer 2000 itself and FortiManager
    uint swap_start, swap_stop;
    uint log_start, fmg_start, limit;

    log_start = sector_offset;
    limit = heads * sectors * cylinders - 1;

    swap_stop = limit;
    swap_start   = (swap_stop - (SWAP_SIZE/sector_size));

    fmg_start = (swap_start - log_start) / 6 * 5;          //1/6 space for FortiManager

    set_partition(0, 0, log_start, fmg_start - 1, LINUX_NATIVE);
    set_partition(1, 0, fmg_start, swap_start - 1, LINUX_NATIVE);
    set_partition(2, 0, swap_start, swap_stop, LINUX_SWAP);

#endif
}

static void write_table(void)
{
    int i;
    for (i = 0; i < 3; i++)
	if (ptes[i].changed)
	    ptes[3].changed = 1;
    for (i = 3; i < partitions; i++) {
	struct pte *pe = &ptes[i];

	if (pe->changed) {
//message("%s:%d:%s: i = %d\n", __FILE__, __LINE__, __FUNCTION__, i);
	    write_part_table_flag(pe->sectorbuffer);
	    write_sector(fd, pe->offset, pe->sectorbuffer);
	}
    }
    reread_partition_table(fd);
}

static void reread_partition_table(int dev)
{
    int error = 0;
    int i;

    sync();
    sleep(2);
    if ((i = ioctl(dev, BLKRRPART)) != 0) {
	error = errno;
    } else {
	/* some kernel versions (1.2.x) seem to have trouble
	   rereading the partition table, but if asked to do it
	   twice, the second time works. - biro@yggdrasil.com */
	sync();
	sleep(2);
	if ((i = ioctl(dev, BLKRRPART)) != 0)
	    error = errno;
    }

    if (i) {
	printf(_("\nWARNING: Re-reading the partition table "
		 "failed with error %d: %s.\n"
		 "The kernel still uses the old table.\n"
		 "The new table will be used " "at the next reboot.\n"), error, strerror(error));
    }

    close(dev);
    sync();
    sleep(4);
}

static int fdisk_main(char *dev)
{
    disk_device = dev;
    get_boot(fdisk);
    add_partitions();
    write_table();		/* does not return */
    return 0;
}

extern int test_device_in_partition_list (const char *dev);
int partition_log_disk(char *dev)
{
    int ret;
    
    if((ret = test_device_in_partition_list(dev)) != 0){
	message("\nThe device %s doesn't exist!\n", dev);
        return ret;
    }

#ifdef HAVE_HW_RAID
    /* Use new partition API which allows >2TB partitions */

    ret = partition_hwraid_logdisk (dev);
#else
    /* For log disks we only encrypt partition table, otherwise access is too
       slow */

    ide_set_encrypt(dev, 1);
    ret = fdisk_main(dev);
    ide_set_encrypt(dev, 0);
#endif
    return ret;
}

#ifdef HAVE_HW_RAID

/*
  Create a log partition and a swap partition if required on the specified
  disk. Creates a GPT partition table, so the kernel must have support for
  EFI GPT tables compiled in.
*/

static int
partition_hwraid_logdisk (char *dev)
{
  hwpart_table_t table;
  struct hd_geometry geo;
  unsigned long disk_size, swap_size;           /* in cylinders */
  unsigned long cyl_size;                       /* in bytes */
  unsigned long sect_size;                      /* in bytes */
  int fd;

  if ((fd = open (dev, O_RDONLY)) < 0)
    {
      message("Can't open %s\n", dev);
      return -1;
    }

  if (ioctl (fd, BLKSSZGET, &sect_size) < 0)
    {
      message("%s: BLKSSZGET failed\n", dev);
      close (fd);
      return -1;
    }

  close (fd);

  /*
    If there is no partition table at all, hwpart_get_diskinfo will fail. So,
    create the table first, which also wipes out existing partitions so we
    start with a clean slate. We create a GPT table, not MSDOS, so we can have
    large partitions.
  */

  if (hwpart_create_table (dev, HWPART_TABLE_GPT) < 0)
    {
      message("%s: can't create partition table\n", dev);
      return -1;
    }

  if (hwpart_get_diskinfo (dev, &geo, table) < 0)
    {
      message("%s: can't get diskinfo\n", dev);
      return -1;
    }

  cyl_size = geo.heads * geo.sectors * sect_size;
  disk_size = geo.cylinders;
  swap_size = (SWAP_SIZE + cyl_size - 1) / cyl_size;

#ifdef HAVE_SWAP_ON_DISK
  if (hwpart_add_partition (dev, 0, disk_size - swap_size) < 0 ||
      hwpart_add_partition (dev, disk_size - swap_size, disk_size))
    return -1;
#else
  if (hwpart_add_partition (dev, 0, disk_size) < 0)
    return -1;
#endif

  return 0;
}
#endif  /* HAVE_HW_RAID */

static struct _LOG_DISK {
	int platform;
	int version;
	char *dev;
	char *mpoint;
} log_disk[] = {
        /* FortiAnalyzer uses a raid array device for logging, and that is started
           and occasionally reformated at configuration load time or later from
           the cli */
        /* TODO:delete this, we don't need it */
#if defined(CONFIG_SYSTEM_FLG_100) 
	{100,1,"/dev/hdc1","/var/log"},
#elif defined(CONFIG_SYSTEM_FLG_100A)
	{101,1,"/dev/hdc1","/var/log"},
#elif defined(CONFIG_SYSTEM_FLG_100B)
	{102,1,"/dev/hdc1","/var/log"},
#elif defined(CONFIG_SYSTEM_FLG_2000)
	{2000,1,"/dev/sda1","/var/log"},
#elif defined(CONFIG_SYSTEM_FLG_2000A)
	{2001,1,"/dev/sda1","/var/log"},
#elif defined(CONFIG_SYSTEM_FLG_4000)
	{4000,1,"/dev/sda1","/var/log"},
#elif defined(CONFIG_SYSTEM_FLG_4000A)
	{4001,1,"/dev/sda1","/var/log"},
#endif /* FLG_100 */
	{200,1,"/dev/hdb","/var/log"},
	{300,1,"/dev/hdc","/var/log"},
#if defined(CONFIG_SYSTEM_FLG_400)
	{400,1,"/dev/md0","/var/log"},
#elif defined(CONFIG_SYSTEM_FLG_800)
	{800,1,"/dev/md0","/var/log"},
#elif defined(CONFIG_SYSTEM_FLG_800B)
	{802,1,"/dev/md0","/var/log"},
#elif defined(CONFIG_SYSTEM_FLG_1000B)
	{1001,1,"/dev/md0","/var/log"},
#else
	{400,1,"/dev/hdc","/var/log"},
#endif
	{400,3,"/dev/hdb","/var/log"},
	{500,1,"/dev/hdc","/var/log"},
	{500,2,"/dev/hdc","/var/log"},
	{500,3,"/dev/hdb","/var/log"},
	{1000,1,"/dev/hdc","/var/log"},
	{2000,1,"/dev/hdc","/var/log"},
	{3000,1,"/dev/hda","/var/log"},
	{3600,1,"/dev/hda","/var/log"},
	{3600,2,"/dev/hdc","/var/log"},
	{0,0,NULL,NULL},
};


char * get_log_disk_info(char **mpoint)
{
	int plat,ver;
	int i=0;
	char *dev=NULL;

	plat = get_pfid_from_bios(&ver);

	while (log_disk[i].dev) {
		if ((log_disk[i].platform == plat) && (log_disk[i].version == ver)) {
			dev = log_disk[i].dev;
			if (mpoint) *mpoint = log_disk[i].mpoint;
			break;
		}
		i++;
	}
	return dev;
}

#if defined(CONFIG_HDFMT) || defined(CONFIG_DISKTOOL)
static struct _DATA_DISK {
	int platform;
	int version;
	char *dev;
} data_disk[] = {
	{50,1,"/dev/hda1"},
	{60,1,"/dev/hda1"},
	{100,1,"/dev/hda1"},
	{101,1,"/dev/hda1"},
	{200,1,"/dev/hda1"},
	{300,1,"/dev/hda1"},
	{400,1,"/dev/hda1"},
	{400,3,"/dev/hda1"},
	{500,1,"/dev/hda1"},
	{500,2,"/dev/hda1"},
	{500,3,"/dev/hda1"},
	{1000,1,"/dev/hda1"},
	{2000,1,"/dev/hda1"},
	{3000,1,"/dev/hdc1"},
	{3600,1,"/dev/hdc1"},
	{3600,2,"/dev/hda1"},
	{0,0,NULL},
};

static void get_data_disk_info(char **dev)
{
	int plat,ver;
	int i=0;

	if (!dev) return;

	plat = get_pfid_from_bios(&ver);

	while (data_disk[i].dev) {
		if ((data_disk[i].platform == plat) && (data_disk[i].version == ver)) {
			*dev = data_disk[i].dev;
			return;
		}
		i++;
	}
	*dev="";
	return;
}
#endif

/*
	op:
	2:	is only used for formatting 2.36 log disk.
	1:	is only used for formatting 2.50 log disk.
	0:	Is for upgrade.
*/
extern int format_logdisk(int op)
{
	char *dev;
	char dev1[20];
	char *args[9];
	int st;
    	pid_t pid;
	static char *envp[] = {
		"TERM=vt100",
	     	"PATH=/bin:/sbin",
     		NULL
    	};

	dev = get_log_disk_info(NULL);
	snprintf(dev1,20,"%s1",dev);
    args[0]= "/bin/mke2fs";
    args[1]= "-j";
    args[2]= "-F";
    args[3] = "-O";
    args[4] = "dir_index";
    args[5] = "-b";
    args[6] = "4096";
    args[7]= dev1;
    args[8]= NULL;


message("YOU SHOULD NOT SEE THIS: %s:%d >>> print_args():\n", __FILE__, __LINE__);
print_args(2, args);
	if (op) {
		message("Create MBR for\n");
		ide_set_encrypt(dev,1);
		fdisk_main(dev);
	}
	message("Enhance log disk...\n");
	if (op!=2) ide_set_encrypt(dev,0);
	if ((pid=fork())==0) {
		execve("/bin/mke2fs", args, envp);
		exit(-1);
	}
	wait(&st);
	if (st==0) message("Enhance log disk successful!\n");
	else message("Enhance log disk failed! status=%d\n",st);
	fflush(NULL);
	if (op) while(1);
	return 0;
}

int probe_logdisk()
{
#if defined(CONFIG_HDFMT) || defined(CONFIG_DISKTOOL)
    char *dev=NULL;
        //	unsigned char buf[512];
    int fd, rt;
    //lock_crash_create();
    fd = get_pfid_from_bios(&rt);
    if (fd<200) return 0;

    get_data_disk_info(&dev);

    if (dev[0]==0) return 0;
    rt = ncfg_mount_fs ( dev, "/data", "ext3", 0, NULL);
    if (rt == NONE_FS) {
	//log_crash("Enhance log disk error:%d\n",__LINE__);
	return 0;
    }
    return 0;
#else
    // see logdisk_cmd_upgrade() in smit.c
    // - this function will check if the upgrade is needed and proceed
    //    accordingly
    int rt = run_command("/bin/smit logdisk upgrade");
    return rt;
#endif /* CONFIG_HDFMT || CONFIG_DISKTOOL */
}

extern int errno;

void ide_set_logdisk_mode(char *dev, int mode)
{
    if (logdiskfg == 1)
	ide_set_encrypt(dev, 1);
    else
	ide_set_encrypt(dev, 0);
}

#ifdef CONFIG_HDFMT
int is_format_logdisk()
{
    int fd, rt;
    unsigned char fg=0;
    fd = open("/data/etc/logdiskfg",O_RDONLY);
    if (fd>=0) {
	rt= read(fd,&fg,1);
	if (rt!=1) fg=0;
	close(fd);
    }
    if (fg!=1) fg=0;
    return fg;
}
#endif

