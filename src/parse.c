#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "client.h"
#include "commands.h"
#include "parse.h"
#include "report.h"
#include "sock.h"
#include "xalloc.h"

#define MAX_ARGUMENTS 30


static inline int is_whitespace(char x)	{
	return ((x == ' ') || (x == '\t') || (x == '\r'));
}

static inline int is_final(char x) {
	return ((x == '\n') || (x == '\0'));
}

static int parse_message(const char *str, Client *c)
{
	typedef enum { ST_INITIAL, ST_WHITESPACE, ST_ARGUMENT, ST_FINAL } State;
	State state = ST_INITIAL;

	int error = 0;
	int pos = 0;
	char *arg_space;
	int argc = 0;
	char *argv[MAX_ARGUMENTS];
	int argpos = 0;
	CommandHandler function = NULL;

	debug(RPT_DEBUG, "%s(str=\"%.120s\", client=[%d])", __FUNCTION__, str, client_get_socket(c));

	arg_space = xmalloc(strlen(str)+1, __FUNCTION__);
	if (arg_space == NULL) {
		sock_answer_unavailable(client_get_socket(c));
		return 0;
	}

	argv[0] = arg_space;

	while ((state != ST_FINAL) && !error) {
		char ch = str[pos++];

		switch (state) {
		  case ST_INITIAL:
		  case ST_WHITESPACE:
			if (is_whitespace(ch))
				break;
			if (is_final(ch)) {
				state = ST_FINAL;
				break;
			}
			/* fall through */
			state = ST_ARGUMENT;
		  case ST_ARGUMENT:
			if (is_final(ch)) {
				if (argc >= MAX_ARGUMENTS-1) {
					error = 1;
				}
				else {
					argv[argc][argpos] = '\0';
					argv[argc+1] = argv[argc] + argpos + 1;
					argc++;
					argpos = 0;
				}
				state = ST_FINAL;
			}
			else if (is_whitespace(ch)) {
				if (argc >= MAX_ARGUMENTS-1) {
					error = 1;
				}
				else {
					argv[argc][argpos] = '\0';
					argv[argc+1] = argv[argc] + argpos + 1;
					argc++;
					argpos = 0;
				}
				state = ST_WHITESPACE;
			}
			else {
				argv[argc][argpos++] = ch;
			}
			break;
		  case ST_FINAL:
		  	/* never reached */
			break;
		}
	}
	if (argc < MAX_ARGUMENTS)
		argv[argc] = NULL;
	else
		error = 1;

	if (error) {
		sock_answer_unavailable(client_get_socket(c));
		report(RPT_WARNING, "Could not parse command from client on socket %d: %.40s", client_get_socket(c));
		free(arg_space);
		return 0;
	}


	/* now execute the command .... */
	function = commands_get_handler(argv[0]);

	if (function != NULL) {
		error = function(c, argc, argv, NULL);
		if (error) {
			sock_answer_unavailable(client_get_socket(c));
			report(RPT_WARNING, "Command function returned an error after command from client on socket %d: %.40s", client_get_socket(c), str);
		}
	} else {
		sock_answer_undefined(client_get_socket(c));
		report(RPT_WARNING, "Invalid command from client on socket %d: %.40s", client_get_socket(c), str);
	}

	free(arg_space);
	return 0;
}


int
parse_all_client_messages(void)
{
	Client *c;

	debug(RPT_DEBUG, "%s()", __FUNCTION__);

	for (c = clients_getfirst(); c != NULL; c = clients_getnext()) {
		char *str;

		for (str = client_get_message(c); str != NULL; str = client_get_message(c)) {

			parse_message(str, c);
			free(str);

			if (client_get_destroy(c)) {
				sock_destroy_client_socket(c);
				break;
			}

			client_reset_elapsed_time(c);
		}
	}
	return 0;
}
