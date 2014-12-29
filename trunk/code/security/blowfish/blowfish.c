/*
 *  the blowfish cipher test program
 *  it used openssl crypto library
 *
 *  write by Forrest.zhang
 */

#include <openssl/blowfish.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>

#define _GNU_SOURCE         1
#include <getopt.h>

enum {
	BF_ECB = 1,
	BF_CBC,
	BF_CFB,
	BF_OFB
};

static int g_mode = -1;
static int g_enc = -1;
static unsigned char g_key[64] = {0};
static int g_keylen = 0;
static unsigned char g_ivec[8] = {0x1, 0x3, 0x5, 0x7, 0x2, 0x4, 0x6, 0x8};

static void inline _usage(void)
{
	printf("blowfish <-e | -d> < -m mode> <key file> <in file> <out file>\n");
	printf("\t-e\tencrypt data\n");
	printf("\t-d\tdecrypt data\n");
	printf("\t-m\tencrypt|decrypt mode: ecb cbc cfb ofb\n");
	printf("\tkey\tthe key of encrypt or decrypt");
	printf("\tin file\tthe input data file\n");
	printf("\tout file\tthe output data file\n");
}


static inline const char *_mode_name(int mode)
{
	switch (mode) {
	case BF_ECB:
		return "ECB";
	case BF_CBC:
		return "CBC";
	case BF_CFB:
		return "CFB";
	case BF_OFB:
		return "OFB";
	default:
		return "Unkowned";
	}
}

static char opts[] = "edm:";
static int inline _getopts(int argc, char **argv)
{
	char c;
	int keyfd;
	struct stat buf;

	opterr = 0;
	while( (c = getopt(argc, argv, opts)) != -1) {
		switch (c) {
		case 'e':
			g_enc = BF_ENCRYPT;
			break;
		case 'd':
			g_enc = BF_DECRYPT;
			break;
		case 'm':
			if (strcmp(optarg, "ecb") == 0)
				g_mode = BF_ECB;
			else if (strcmp(optarg, "cbc") == 0)
				g_mode = BF_CBC;
			else if (strcmp(optarg, "cfb") == 0)
				g_mode = BF_CFB;
			else if (strcmp(optarg, "ofb") == 0)
				g_mode = BF_OFB;
			else {
				printf("error mode\n");
				return -1;
			}
			break;
		default:
			return -1;
		}
		
	}
	
	/* no mode option */
	if (g_mode == -1) {
		printf("no mode, must using: -m <ecb | cbc | cfb | ofb>\n");
		return -1;
	}

	/* no encypt or decrypt */
	if (g_enc == -1) {
		printf("must using -d to decrypt or -e to encrypt\n");
		return -1;
	}
	
	/* not enough argument */
	if (argc - optind != 3)
		return -1;

	/* open key file and store key in @key, key length must multiplies of 32bits */
	keyfd = open(argv[optind], O_RDONLY);
	if (keyfd < 0) {
		printf("Can't open key file %s\n", argv[optind]);
		return -1;
	}
	
	if (fstat(keyfd, &buf)) {
		printf("Can't stat key file %s\n", argv[optind]);
		return -1;
	}

	if (buf.st_size % 4 != 0 ||
	    buf.st_size > 56 ||
	    buf.st_size < 4) {
		printf("Blowfish key must be multiplies of 32bits, not exceed 14 32bits\n");
		return -1;
	}

	g_keylen = buf.st_size;
	if (g_keylen != read(keyfd, g_key, g_keylen)) {
		printf("Read key file %s error\n", argv[optind]);
		return -1;
	}

	/* is data file exist */
	if (access(argv[optind + 1], R_OK)) {
		printf("can't open file %s\n", argv[optind + 1]);
		return -1;
	}

	return 0;
}

static inline void _ecb_encrypt(BF_KEY *key, const char *infile, const char *outfile)
{
	unsigned char src[8] = {0};
	unsigned char des[8] = {0};
	int infd, outfd;
	int n;

	infd = open(infile, O_RDONLY);
	if (infd < 0) {
		printf("can't open source data file %s\n", infile);
		return;
	}

	outfd = open(outfile, O_WRONLY | O_CREAT, 0644);
	if (outfd < 0) {
		printf("can't open result data file %s\n", outfile);
		return;
	}

	do {
		n = read(infd, src, 8);
		if (n != 8)
			goto out;

		BF_ecb_encrypt(src, des, key, g_enc);
		
		n = write(outfd, des, 8);
		if (n != 8)
			goto out;
	} while (1);

 out:
	close(infd);
	close(outfd);

}

static inline void _cbc_encrypt(BF_KEY *key, const char *infile, const char *outfile)
{
	unsigned char *src;
	unsigned char *des;
	struct stat st;
	int len;
	int infd = -1, outfd = -1;

	infd = open(infile, O_RDONLY);
	if (infd < 0) {
		printf("Open input file %s error\n", infile);
		return;
	}

	outfd = open(outfile, O_WRONLY | O_CREAT, 0644);
	if (outfd < 0) {
		printf("Open output file %s error\n", outfile);
		goto out;
	}

	if (fstat(infd, &st)) {
		printf("Get input file %s stat error\n", infile);
		goto out;
	}

	len = st.st_size;
	src = malloc(len + 1);
	if (! src) {
		printf("Can't alloc %d bytes memory\n", len + 1);
		goto out;
	}
	memset(src, 0, len + 1);

	des = malloc(len + 1);
	if (! des) {
		printf("Can't alloc %d bytes memory\n", len + 1);
		goto out;
	}
	memset(des, 0, len + 1);

	if (read(infd, src, len) != len) {
		printf("Read input file %s error\n", infile);
		goto out;
	}

	BF_cbc_encrypt(src, des, len, key, g_ivec, g_enc);
	if (write(outfd, des, len) != len) {
		printf("Write result to output file %s error\n", outfile);
		goto out;
	}

 out:
	if (infd >= 0)
		close(infd);
	
	if (outfd >= 0)
		close(outfd);
}

static inline void _cfb_encrypt(BF_KEY *key, const char *infile, const char *outfile)
{
	unsigned char *src;
	unsigned char *des;
	struct stat st;
	int len;
	int num = 0;
	int infd = -1, outfd = -1;

	infd = open(infile, O_RDONLY);
	if (infd < 0) {
		printf("Open input file %s error\n", infile);
		return;
	}

	outfd = open(outfile, O_WRONLY | O_CREAT, 0644);
	if (outfd < 0) {
		printf("Open output file %s error\n", outfile);
		goto out;
	}

	if (fstat(infd, &st)) {
		printf("Get input file %s stat error\n", infile);
		goto out;
	}

	len = st.st_size;
	src = malloc(len + 1);
	if (! src) {
		printf("Can't alloc %d bytes memory\n", len + 1);
		goto out;
	}
	memset(src, 0, len + 1);

	des = malloc(len + 1);
	if (! des) {
		printf("Can't alloc %d bytes memory\n", len + 1);
		goto out;
	}
	memset(des, 0, len + 1);

	if (read(infd, src, len) != len) {
		printf("Read input file %s error\n", infile);
		goto out;
	}

	BF_cfb64_encrypt(src, des, len, key, g_ivec, &num, g_enc);
	if (write(outfd, des, len) != len) {
		printf("Write result to output file %s error\n", outfile);
		goto out;
	}

 out:
	if (infd >= 0)
		close(infd);
	
	if (outfd >= 0)
		close(outfd);
}

static inline void _ofb_encrypt(BF_KEY *key, const char *infile, const char *outfile)
{
	unsigned char *src;
	unsigned char *des;
	struct stat st;
	int len;
	int num = 0;
	int infd = -1, outfd = -1;

	infd = open(infile, O_RDONLY);
	if (infd < 0) {
		printf("Open input file %s error\n", infile);
		return;
	}

	outfd = open(outfile, O_WRONLY | O_CREAT, 0644);
	if (outfd < 0) {
		printf("Open output file %s error\n", outfile);
		goto out;
	}

	if (fstat(infd, &st)) {
		printf("Get input file %s stat error\n", infile);
		goto out;
	}

	len = st.st_size;
	src = malloc(len + 1);
	if (! src) {
		printf("Can't alloc %d bytes memory\n", len + 1);
		goto out;
	}
	memset(src, 0, len + 1);

	des = malloc(len + 1);
	if (! des) {
		printf("Can't alloc %d bytes memory\n", len + 1);
		goto out;
	}
	memset(des, 0, len + 1);

	if (read(infd, src, len) != len) {
		printf("Read input file %s error\n", infile);
		goto out;
	}

	BF_ofb64_encrypt(src, des, len, key, g_ivec, &num);
	if (write(outfd, des, len) != len) {
		printf("Write result to output file %s error\n", outfile);
		goto out;
	}

 out:
	if (infd >= 0)
		close(infd);
	
	if (outfd >= 0)
		close(outfd);
}

int main(int argc, char **argv)
{
	BF_KEY key;
       
	/* verify command line parameter */
	if (_getopts(argc, argv)) {
		_usage();
		return -1;
	}

	if (g_enc == BF_ENCRYPT)
		printf("encrypt data using %s mode...\n", _mode_name(g_mode));
	else if(g_enc == BF_DECRYPT)
		printf("decrypt data using %s mode...\n", _mode_name(g_mode));

	BF_set_key(&key, g_keylen, g_key);

	switch (g_mode) {
	case BF_ECB:
		_ecb_encrypt(&key, argv[argc - 2], argv[argc - 1]);
		break;
	case BF_CBC:
		_cbc_encrypt(&key, argv[argc - 2], argv[argc - 1]);
		break;
	case BF_CFB:
		_cfb_encrypt(&key, argv[argc - 2], argv[argc - 1]);
		break;
	case BF_OFB:
		_ofb_encrypt(&key, argv[argc - 2], argv[argc - 1]);
		break;
	}

	return 0;
}


