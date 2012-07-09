/*
 *	@file	saxtest.c
 *	@brief	a simple test program of libXML SAX parser
 *
 *	@date
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <libxml/parser.h>

static char *g_buffer = NULL;
static size_t g_buflen = 0;
static char g_file[1024];

static void _usage(void)
{
	printf("saxtest <XML filename>\n");
}

static int _parse_cmd(int argc, char **argv)
{
	if (argc != 2)
		return -1;

	memcpy(g_file, argv[1], 1023);
	
	if (access(g_file, R_OK)) {
		printf("can't read file %s\n", g_file);
		return -1;
	}

	return 0;
}

static int _read_xml_file(const char *filename)
{
	int fd;
	struct stat stat;

	if (!filename)
		return -1;

	fd = open(filename, O_RDONLY);
	if (fd < 0) 
		return -1;
	
	if (fstat(fd, &stat)) {
		close(fd);
		return -1;
	}

	g_buflen = stat.st_size;
	g_buffer = malloc(g_buflen);
	if (!g_buffer) {
		close(fd);
		return -1;
	}

	read(fd, g_buffer, g_buflen);
	close(fd);

	return 0;
}

static int _init(void)
{
	if (_read_xml_file(g_file))
		return -1;

	return 0;
}

static int _release(void)
{
	if (g_buffer)
		free(g_buffer);

	return 0;
}

static void _element_start(
	void *ctx, 
	const xmlChar *name, 
	const xmlChar **attrs)
{
	int i = 0;
	
	printf("---> element %s start\n", name);
	
	while(attrs && attrs[i] && attrs[i + 1]) {
		printf("     attribute %s = %s\n", 
		       attrs[i], attrs[i + 1]);
		i += 2;
	}
}

static void _element_end(
	void *ctx,
	const xmlChar *name)
{
	printf("<--- element %s end\n", name);
}


int main(int argc, char **argv)
{
	xmlSAXHandler handler;
	int fd;
	int mid;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_init())
		return -1;

	memset(&handler, 0, sizeof(xmlSAXHandler));

	handler.startElement = _element_start;
	handler.endElement = _element_end;

	if (xmlSAXUserParseMemory(&handler, NULL, g_buffer, g_buflen) < 0) {
		printf("SAX parser error 1\n");
	}

	xmlCleanupParser();

	_release();

	return 0;
}
