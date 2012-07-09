/*
 * @file
 * @brief
 *
 * @author
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <getopt.h>
#include <string.h>
#include <sys/stat.h>

static long g_start;
static long g_size;
static char *g_infile;
static char *g_outfile;

static void _usage(void)
{
	printf("fwrite: write a file to another file\n");
	printf("fwrite [options] <file 1> <file2>\n");
	printf("\t-s\tstart postion\n");
	printf("\t-c\twrite bytes\n");
	printf("\t-h\thelp info\n");
}

static int _parse_cmd(int argc, char **argv)
{
	char c;
	char opt[] = ":s:c:h";
	
	opterr = 0;
	while ( (c = getopt(argc, argv, opt)) != -1) {
		switch (c) {
			
		case 's':
			g_start = atoi(optarg);
			if (g_start <= 0) {
				printf("-s <1 - N> need positive number\n");
				return -1;
			}
			break;
			
		case 'c':
			g_size = atoi(optarg);
			if (g_size <= 0) {
				printf("-c <1 - N> need positive number\n");
				return -1;
			}
			break;
			
		case 'h':
			return -1;

		case ':':
			printf("option %c missing argument\n", optopt);
			return -1;

		case '?':
			printf("unknowed option %c\n", optopt);
			return -1;
		}
	}

	if (argc - optind != 2)
		return -1;

	g_infile = argv[optind];
	if (access(g_infile, R_OK)) {
		printf("can't read file %s\n", g_infile);
		return -1;
	}

	g_outfile = argv[optind + 1];
	if (access(g_outfile, W_OK)) {
		printf("can't write file %s\n", g_outfile);
		return -1;
	}

	return 0;
}


int main(int argc, char **argv)
{
	int infd, outfd;
	char buf[513] = {0};
	long n, m, l, k;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	infd = open(g_infile, O_RDONLY);
	if (infd < 0) {
		printf("open file %s read error: %s\n", 
		       g_infile, strerror(errno));
		return -1;
	}

	outfd = open(g_outfile, O_WRONLY);
	if (outfd < 0) {
		printf("open file %s write error: %s\n",
		       g_outfile, strerror(errno));
		close(infd);
		return -1;
	}

	lseek(outfd, g_start, SEEK_SET);

	if (g_size > 0) {
		n = g_size;
	}
	else {
		struct stat stats;
		if (fstat(infd, &stats)) {
			printf("fstat file %s error: %s\n",
			       g_infile, strerror(errno));
			close(infd);
			close(outfd);
			return -1;
		}
		n = stats.st_size;
	}

	l = n;
	k = 0;
	while (l > 0) {
		if (l > 512)
			m = read(infd, buf, 512);
		else
			m = read(infd, buf, l);
		if (m < 0) {
			printf("read %s error: %s\n", g_infile, strerror(errno));
			break;
		}
		else if (m == 0)
			break;
		n = write(outfd, buf, m);
		if (n != m) {
			printf("write %s error: %s\n", g_outfile, strerror(errno));
			break;
		}
		l -= m;
		k += m;
	}

	close(infd);
	close(outfd);

	printf("write %ld bytes from %s to %s start at %ld\n",
	       k, g_infile,  g_outfile, g_start);

	return 0;
}

