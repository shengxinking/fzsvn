/**
 *	@file	
 *
 *	@brief
 *	
 *	@date
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>

#define	CMD_NAMELEN	15
#define COM_NAMELEN	31

static int _g_done = 0;

typedef int (*_cmd_func_t)(const char *msg);

typedef struct command {
	char		name[CMD_NAMELEN + 1];
	_cmd_func_t	func;
	char		comment[COM_NAMELEN + 1];
} command_t;

static int _cmd_config(const char *msg);
static int _cmd_edit(const char *msg);
static int _cmd_show(const char *msg);
static int _cmd_quit(const char *msg);
static char **_cmd_completion(const char *text, int start, int end);
static char *_cmd_generator(const char *text, int state);

command_t commands[] = {
	{"config", _cmd_config, "Configure a object"},
	{"cd", _cmd_config, "Configure a object"},
	{"edit", _cmd_edit, "Edit a new object"},
	{"show", _cmd_show, "Show the value of object"},
	{"quit", _cmd_quit, "Quit the program"},
};

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
}

/**
 *	Parse command line argument.	
 *
 * 	Return 0 if parse success, -1 on error.
 */
static int 
_parse_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'h':
			return -1;

		case ':':
			printf("Option %c missing argument\n", optopt);
			return -1;

		case '?':
			printf("Unknowed option %c\n", optopt);
			return -1;
		}
	}

	if (argc != optind)
		return -1;

	return 0;
}

/**
 *	Init some global resource used in program.	
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_initiate(void)
{
	rl_attempted_completion_function = _cmd_completion;

	return 0;
}


/**
 *	Release global resource alloced by _initiate().	
 *
 * 	No Return.
 */
static void 
_release(void)
{
	clear_history();
}

static int 
_cmd_config(const char *arg)
{
	printf("config %s\n", arg);
	return 0;
}

static int 
_cmd_edit(const char *arg) 
{
	printf("edit %s\n", arg);
	return 0;
}

static int 
_cmd_quit(const char *arg) 
{
	printf("quit %s\n", arg);

	_g_done = 1;
	return 0;
}

static int 
_cmd_show(const char *arg) 
{
	printf("show %s\n", arg);
	return 0;
}

static char ** 
_cmd_completion(const char *text, int start, int end)
{
	char **matches = NULL;


	matches = rl_completion_matches(text, _cmd_generator);

	printf("%s: text %s, start %d, end %d, matches %p\n", 
	       __func__, text, start, end, matches);

	return matches;
} 

static char *
_dupstr(const char *str)
{
	char *dup;

	if (!str || strlen(str) < 1)
		return NULL;

	dup = malloc(strlen(str) + 1);
	if (!dup)
		return NULL;

	strncpy(dup, str, strlen(str));
	dup[strlen(str)] = 0;

	return dup;
}


static char *
_cmd_generator(const char *text, int state)
{
	static int index, len;
	char *name;

//	printf("%s: test %s, state %d\n", 
//	       __func__, text, state);
	
	if (!state) {
		index = 0;
		len = strlen(text);
	}

	while ( (name = commands[index].name) ) {
		index++;
		if (index > sizeof(commands)/sizeof(command_t))
			return NULL;

		if (strncmp(name, text, len) == 0)
			return (_dupstr(name));
	}

	return NULL;
}

static command_t *
_cmd_find(const char *cmd)
{
	int i;
	int n;
	
	if (!cmd)
		return NULL;

	n = sizeof(commands) / sizeof(command_t);
	for (i = 0; i < n; i++) {
		if (strcmp(cmd, commands[i].name) == 0)
			return &(commands[i]);
	}

	return NULL;
}

static int 
_cmd_run(const char *line)
{
	char *cmd = NULL;
	char *next = NULL;
	command_t *command = NULL;
	char *buf = NULL;

	if (!line)
		return -1;

	buf = _dupstr(line);

	printf("run: %s\n", buf);

	cmd = strtok_r(buf, " ", &next);
	
	command = _cmd_find(cmd);
	if (command) {
		command->func(next);
	}

	free(buf);
	
	return 0;
}

/**
 *	The main entry of program.	
 *
 * 	Return 0 if success, other value on error.
 */
int 
main(int argc, char **argv)
{
	char *line = NULL;
	int ret = 0;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	for (; _g_done == 0; ) {

		line = readline("RL: ");

		if (!line)
			break;

		ret = _cmd_run(line);
		
		if (ret == 0) {
			add_history(line);
		}
		
		free(line);
	}

	_release();

	return 0;
}



