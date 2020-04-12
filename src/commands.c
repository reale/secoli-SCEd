#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "commands.h"
#include "main.h"  /* VERSION, PROTOCOL_VERSION */
#include "node.h"
#include "options.h"
#include "report.h"
#include "search.h"
#include "session.h"
#include "sock.h"
#include "topology.h"
#include "user.h"
#include "utils.h"


static int alter_handler(Client *c, int argc, char **argv, Session *s);
static int announce_handler(Client *c, int argc, char **argv, Session *s);
static int authenticate_handler(Client *c, int argc, char **argv, Session *s);
static int create_handler(Client *c, int argc, char **argv, Session *s);
static int deauthenticate_handler(Client *c, int argc, char **argv, Session *s);
static int destroy_handler(Client *c, int argc, char **argv, Session *s);
static int dump_handler(Client *c, int argc, char **argv, Session *s);
static int echo_handler(Client *c, int argc, char **argv, Session *s);
static int fetch_handler(Client *c, int argc, char **argv, Session *s);
static int find_handler(Client *c, int argc, char **argv, Session *s);
static int get_handler(Client *c, int argc, char **argv, Session *s);
static int hello_handler(Client *c, int argc, char **argv, Session *s);
static int list_handler(Client *c, int argc, char **argv, Session *s);
static int negotiate_handler(Client *c, int argc, char **argv, Session *s);
static int quit_handler(Client *c, int argc, char **argv, Session *s);
static int set_handler(Client *c, int argc, char **argv, Session *s);
static int show_handler(Client *c, int argc, char **argv, Session *s);
static int shutdown_handler(Client *c, int argc, char **argv, Session *s);
static int who_handler(Client *c, int argc, char **argv, Session *s);
static int within_handler(Client *c, int argc, char **argv, Session *s);

static int command_is_embeddable(const char *cmd);
static int find_command_handler(const char *cmd);
static int find_get_handler(Client *c, int argc, char **argv, Session *s, int option_scope);
static Session *get_effective_session(Client *c, Session *s);
static int print_search_result_count(Client *c, Session *s);
static int print_search_result(Client *c, Session *s, bool print_result_count);
static int slurp_and_set_options(Client *c, int argc, char **argv, Session *s, int scope);
static char *slurp_name(int argc, char **argv);
static char *slurp_zipcode(int argc, char **argv);
static char *slurp_mae_code(int argc, char **argv);

static command_handler commands[] = {
	{ "ALTER",            alter_handler            , 0 },
	{ "ANNOUNCE",         announce_handler         , 0 },
	{ "AUTHENTICATE",     authenticate_handler     , 0 },
	{ "CREATE",           create_handler           , 0 },
	{ "DEAUTHENTICATE",   deauthenticate_handler   , 0 },
	{ "DESTROY",          destroy_handler          , 0 },
	{ "DUMP",             dump_handler             , 1 },
	{ "ECHO",             echo_handler             , 1 },
	{ "FIND",             find_handler             , 1 },
	{ "FETCH",            fetch_handler            , 1 },
	{ "GET",              get_handler              , 1 },
	{ "HELLO",            hello_handler            , 1 },
	{ "LIST",             list_handler             , 0 },
	{ "NEGOTIATE",        negotiate_handler        , 0 },
	{ "QUIT",             quit_handler             , 0 },
	{ "SET",              set_handler              , 1 },
	{ "SHOW",             show_handler             , 1 },
	{ "SHUTDOWN",         shutdown_handler         , 1 },
	{ "WHO",              who_handler              , 1 },
	{ "WITHIN",           within_handler           , 0 },
	{ NULL,               NULL                     , 0 }
};



static int
find_command_handler(const char *cmd)
{
	int i;

	if (!cmd)
		return -1;

	for (i = 0; commands[i].keyword != NULL; i++) {
		if (strcmp(cmd, commands[i].keyword) == 0)
			return i;
	}

	return -1;
}

static int
command_is_embeddable(const char *cmd)
{
	int i = find_command_handler(cmd);

	if (i < 0)
		return 0;  /* False */
	else
		return commands[i].is_embeddable;
}

CommandHandler
commands_get_handler(const char *cmd)
{
	int i = find_command_handler(cmd);

	if (i < 0)
		return NULL;
	else
		return commands[i].function;
}


static int
alter_handler(Client *c, int argc, char **argv, Session *s)
{
	if (argc != 3 || slurp_and_set_options(c, argc-1, argv+1, s, OPTION_SCOPE_CLIENT) < 2) {
		sock_answer_undefined(client_get_socket(c));
		return 0;
	}

	sock_answer_ok(client_get_socket(c));

	return 0;
}

static int
announce_handler(Client *c, int argc, char **argv, Session *s)
{
	Node *node;

	/*  ANNOUNCE NODE Address PORT Port */

	if (argc != 5 || strcmp(argv[1], "NODE") || strcmp(argv[3], "PORT")) {
		sock_answer_undefined(client_get_socket(c));
		return 0;
	}

	if (!client_get_user(c) || !topology_leader()) {
		sock_answer_unavailable(client_get_socket(c));
		return 0;
	}

	node = node_assemble(argv[2], argv[4]);
	if (!node) {
		// XXX errors
		sock_answer_undefined(client_get_socket(c));
		return 0;
	}

	if (!topology_announce_node(node)) {
		// XXX errors
		node_destroy(node);
		sock_answer_unavailable(client_get_socket(c));
		return 0;
	}

	sock_answer_ok(client_get_socket(c));

	return 0;
}

static int
authenticate_handler(Client *c, int argc, char **argv, Session *s)
{
	User *user;

	/* AUTHENTICATE USER UserName WITH PASSWORD Password */

	if (argc != 6 || strcmp(argv[1], "USER") || strcmp(argv[3], "WITH") || strcmp(argv[4], "PASSWORD")) {
		sock_answer_undefined(client_get_socket(c));
		return 0;
	}

	user = user_authenticate(argv[2], argv[5]);
	if (!user) {
		sock_answer_unavailable(client_get_socket(c));
		return 0;
	}

	user_deauthenticate(client_get_user(c));
	client_set_user(c, user);

	sock_answer_ok(client_get_socket(c));

	return 0;
}

static int
create_handler(Client *c, int argc, char **argv, Session *s)
{
	Session *session;

	if (argc != 2 || strcmp(argv[1], "SESSION")) {
		sock_answer_undefined(client_get_socket(c));
		return 0;
	}

	session = session_create();

	if (!session || client_add_session(c, session) != 0)
		return -1;

	sock_printf(client_get_socket(c), "%s\n", session_id_to_string(session));

	return 0;
}

static int
deauthenticate_handler(Client *c, int argc, char **argv, Session *s)
{
	User *user;

	/* DEAUTHENTICATE USER */

	if (argc != 2 || strcmp(argv[1], "USER")) {
		sock_answer_undefined(client_get_socket(c));
		return 0;
	}

	user = client_get_user(c);

	user_deauthenticate(user);

	client_set_user(c, NULL);

	sock_answer_ok(client_get_socket(c));

	return 0;
}

static int
destroy_handler(Client *c, int argc, char **argv, Session *s)
{
	Session *session;

	if (argc != 3 || strcmp(argv[1], "SESSION")) {
		sock_answer_undefined(client_get_socket(c));
		return 0;
	}

	session = sessions_find_by_id(client_get_sessions(c), session_string_to_id(argv[2]));
	if (!session) {
		sock_answer_undefined(client_get_socket(c));
		return 0;
	}

	client_remove_session(c, session);

	session_destroy(session);

	sock_answer_ok(client_get_socket(c));

	return 0;
}

static int
print_search_result_count(Client *c, Session *s)
{
	int result_count;

	if (!c || !s) {
		/* TODO: report error */
		sock_answer_unavailable(client_get_socket(c));
		return -1;
	}
	
	result_count = session_count_search_results(s);
	if (result_count < 0) {
		sock_answer_unavailable(client_get_socket(c));
		return 0;
	}
		
	sock_printf(client_get_socket(c), "%d\n", result_count);

	return 0;
}

static int
print_search_result(Client *c, Session *s, bool print_result_count)
{
	Search *search;
	int found = 0;

	if (!c || !s) {
		/* TODO: report error */
		sock_answer_unavailable(client_get_socket(c));
		return -1;
	}
	
	search = session_get_search(s);
	if (!search) {
		/* TODO: report error */
		sock_answer_unavailable(client_get_socket(c));
		return -1;
	}

	found = client_run_output_handler(c, search, print_result_count);

	session_destroy_search(s);

	if (found <= 0) {
		sock_answer_undefined(client_get_socket(c));
		return 0;
	}

	return 0;
}

static int
dump_handler(Client *c, int argc, char **argv, Session *s)
{
	Search *search;
	int options_argc = 0;


	s = get_effective_session(c, s);

	if (argc < 2 || strcmp(argv[1], "LOCATIONS")) {
		sock_answer_undefined(client_get_socket(c));
		return 0;
	}

	if (session_create_search(s) < 0) {
		sock_answer_unavailable(client_get_socket(c));
		return 0;
	}

	if (argc > 2 && !strcmp(argv[2], "WITH")) {
		options_argc = slurp_and_set_options(c, argc-3, argv+3, s, OPTION_SCOPE_SEARCH);
		options_argc += 1;  /* +1 accounts for 'WITH' */
	}

	options_argc += 1;  /* +1 accounts for 'LOCATIONS' */
	argc -= options_argc;
	argv += options_argc;

	search = session_get_search(s);
	if (!search) {
		sock_answer_unavailable(client_get_socket(c));
		return 0;
	}
	
	search_set_type(search, SEARCH_TYPE_DUMP);

	if (session_execute_search(s) < 0) {
		session_destroy_search(s);
		sock_answer_unavailable(client_get_socket(c));
		return 0;
	}

	return print_search_result(c, s, true);
}

static int
echo_handler(Client *c, int argc, char **argv, Session *s)
{
	/* only prints out the first argument, if present  */

	if (argc > 1)
		sock_printf(client_get_socket(c), "%s\n", argv[1]);
	else
		sock_printf(client_get_socket(c), "\n");

	return 0;
}

static int
fetch_handler(Client *c, int argc, char **argv, Session *s)
{ 
	s = get_effective_session(c, s);

	return print_search_result(c, s, false);
}

static int
slurp_and_set_options(Client *c, int argc, char **argv, Session *s, int scope)
{
	if (argc < 2)
		return 0;

	if (option_set(argv[0], argv[1], c, s, scope) < 0) {
		/* TODO: report error */
		return 0;
	}

	if (argc > 3 && !strcmp(argv[2], "AND")) {
		/* recursively parse other options  */
		return slurp_and_set_options(c, argc-3, argv+3, s, scope) + 3;
	} else {
		return 2;
	}
}

static char *
slurp_name(int argc, char **argv)
{
	return flatten_argv(argc, argv);
}

static char *
slurp_zipcode(int argc, char **argv)
{
	return flatten_argv(argc, argv);
}

static char *
slurp_mae_code(int argc, char **argv)
{
	return flatten_argv(argc, argv);
}

static int
find_get_handler(Client *c, int argc, char **argv, Session *s, int option_scope)
{
	int used_argc = 0;
	int options_argc = 0;
	char *name = NULL, *zipcode = NULL, *sce_code = NULL, *mae_code = NULL;
	Search *search;
	int search_type;


	if (argc < 2
		|| (strcmp(argv[0], "LOCATIONS")
		    && strcmp(argv[0], "CONSULATES") 
		    && strcmp(argv[0], "CONSULAR") 
		    && strcmp(argv[0], "SPECIMINA"))) {
		return -1;  /* undefined */
	}

	/* LOCATIONS */
	if (!strcmp(argv[0], "LOCATIONS")) {
		used_argc = 1;
		search_type = SEARCH_TYPE_LOCATIONS;
	/* CONSULATES */
	} else if (!strcmp(argv[0], "CONSULATES")) {
		used_argc = 1;
		search_type = SEARCH_TYPE_CONSULATES;
	/* CONSULAR DISTRICTS */
	} else if (!strcmp(argv[0], "CONSULAR")) {
		if (!strcmp(argv[1], "DISTRICTS")) {
			used_argc = 2;
			search_type = SEARCH_TYPE_CONSULAR_DISTRICTS;
		}
	/* SPECIMINA */
	} else if (!strcmp(argv[0], "SPECIMINA")) {
		if (argc < 3 || strcmp(argv[1], "OF") || strcmp(argv[2], "ZIPCODES"))
			return -1;  /* undefined */
		used_argc = 3;
		search_type = SEARCH_TYPE_ZC_SPECIMINA;
	} else
		/* should we ever get here, something has gone wild ... */
		abort();
		
	if (session_create_search(s) < 0)
		return -2;  /* system unavailable */

	if (argc > used_argc+1 && !strcmp(argv[used_argc], "WITH")) {
		options_argc = slurp_and_set_options(c, argc-(used_argc+1), argv+(used_argc+1), s, option_scope);
		options_argc += 1;  /* +1 accounts for 'WITH' */
	}

	options_argc += used_argc;  /* +1 accounts for 'LOCATIONS', 'CONSULATES', etc.  */
	argc -= options_argc;
	argv += options_argc;

	search = session_get_search(s);
	if (!search)
		return -2;
	
	search_set_type(search, search_type);

	if (argc > 2 && !strcmp(argv[0], "BY") && !strcmp(argv[1], "NAME")) {
		name = slurp_name(argc-2, argv+2);
		if (search_set_name(search, name) < 0) {
			session_destroy_search(s);
			return -2;  /* system unavailable */
		}
	} else if (argc > 2 && !strcmp(argv[0], "BY") && !strcmp(argv[1], "ZIPCODE")) {
		zipcode = slurp_zipcode(argc-2, argv+2);
		if (search_set_zipcode(search, zipcode) < 0) {
			session_destroy_search(s);
			return -2;  /* system unavailable */
		}
	} else if (argc > 2 && !strcmp(argv[0], "BY") && !strcmp(argv[1], "MAE_CODE")) {
		mae_code = slurp_mae_code(argc-2, argv+2);
		if (search_set_mae_code(search, mae_code) < 0) {
			session_destroy_search(s);
			return -2;  /* system unavailable */
		}
	} else if (argc > 2 && !strcmp(argv[0], "BY") && (!strcmp(argv[1], "SCE_CODE") || !strcmp(argv[1], "LOCATION_CODE"))) {
		sce_code = argv[2];
		if (search_set_sce_code(search, sce_code) < 0) {
			session_destroy_search(s);
			return -2;  /* system unavailable */
		}
	} else if (search_type == SEARCH_TYPE_CONSULATES || search_type == SEARCH_TYPE_ZC_SPECIMINA) {
		;  /* BY clause is ignored */
	} else {
		session_destroy_search(s);
		return -1;  /* undefined */
	}

	if (option_scope & OPTION_SCOPE_SESSION) {
		if (session_refresh_search_options(s, search) < 0) {
			session_destroy_search(s);
			return -2;  /* system unavailable */
		}
	}

	if (session_execute_search(s) < 0) {
		session_destroy_search(s);
		return -2;  /* system unavailable */
	}

	return 0;  /* OK */
}

static int
find_handler(Client *c, int argc, char **argv, Session *s)
{
	int error;

	s = get_effective_session(c, s);

	if (argc < 2) {
		sock_answer_undefined(client_get_socket(c));
		return 0;
	}

	error = find_get_handler(c, argc-1, argv+1, s, OPTION_SCOPE_SESSION);
	if (error == -2) {
		sock_answer_unavailable(client_get_socket(c));
		return 0;
	} else if (error < 0) {
		sock_answer_undefined(client_get_socket(c));
		return 0;
	}

	return print_search_result_count(c, s);
}

static int
get_handler(Client *c, int argc, char **argv, Session *s)
{
	int error;

	s = get_effective_session(c, s);

	if (argc < 2) {
		sock_answer_undefined(client_get_socket(c));
		return 0;
	}

	error = find_get_handler(c, argc-1, argv+1, s, OPTION_SCOPE_SEARCH);
	if (error == -2) {
		sock_answer_unavailable(client_get_socket(c));
		return 0;
	} else if (error < 0) {
		sock_answer_undefined(client_get_socket(c));
		return 0;
	}

	return print_search_result(c, s, true);
}

static int
hello_handler(Client *c, int argc, char **argv, Session *s)
{
	/* additional arguments are ignored */

	sock_printf(client_get_socket(c), SOCK_ANSWER_HELLO, VERSION, PROTOCOL_VERSION);

	return 0;
}

#define LIST_HANDLER_LIST_OPTIONS  1
#define LIST_HANDLER_LIST_NODES    2

static int
list_handler(Client *c, int argc, char **argv, Session *s)
{
	int list_handler_list;
	int option_scope;
	char *buffer;
	int i;

	s = get_effective_session(c, s);

	if (argc == 2 && !strcmp(argv[1], "NODES")) {
		list_handler_list = LIST_HANDLER_LIST_NODES;
	} else if (argc == 2 && !strcmp(argv[1], "OPTIONS")) {
		list_handler_list = LIST_HANDLER_LIST_OPTIONS;
		option_scope = OPTION_SCOPE_SESSION;
	} else if (argc == 3 && !strcmp(argv[1], "GLOBAL") && !strcmp(argv[2], "OPTIONS")) {
		list_handler_list = LIST_HANDLER_LIST_OPTIONS;
		option_scope = OPTION_SCOPE_CLIENT;
	} else {
		sock_answer_undefined(client_get_socket(c));
		return 0;
	}

	if (list_handler_list == LIST_HANDLER_LIST_NODES) {
		int node_count = topology_count_nodes();

		if (!client_get_user(c) || node_count < 0) {
			sock_answer_unavailable(client_get_socket(c));
			return 0;
		}

		if (node_count == 0) {
			sock_printf(client_get_socket(c), "NO NODES AS YET\n");
			return 0;
		}
	}

	for (i = 0 ; ; i++) {
		if (list_handler_list == LIST_HANDLER_LIST_NODES)
			buffer = topology_list_nodes(i);
		else if (list_handler_list == LIST_HANDLER_LIST_OPTIONS)
			buffer = options_list(c, s, option_scope, i);

		if (!buffer)
			break;
		sock_printf(client_get_socket(c), "%s\n", buffer);
		free(buffer);
	}

	return 0;
}

static int
negotiate_handler(Client *c, int argc, char **argv, Session *s)
{
	/* additional arguments are ignored */

	if (argc != 4 || strcmp(argv[1], "PROTOCOL") || strcmp(argv[2], "VERSION")) {
		sock_answer_undefined(client_get_socket(c));
		return 0;
	}

 	if (strcmp(argv[3], PROTOCOL_VERSION)) {
		sock_answer_unavailable(client_get_socket(c));
		return 0;
	}

	sock_answer_ok(client_get_socket(c));

	return 0;
}

static int
quit_handler(Client *c, int argc, char **argv, Session *s)
{
	/* additional arguments are ignored */

	if (c != NULL) {
		debug(RPT_INFO, "%s(sock=%i) quits", client_get_socket(c));
		client_set_destroy(c, 1);
	}	

	return 0;
}

static int
set_handler(Client *c, int argc, char **argv, Session *s)
{
	s = get_effective_session(c, s);

	if (argc != 3 || slurp_and_set_options(c, argc-1, argv+1, s, OPTION_SCOPE_SESSION) < 2) {
		sock_answer_undefined(client_get_socket(c));
		return 0;
	}

	sock_answer_ok(client_get_socket(c));

	return 0;
}

static int
show_handler(Client *c, int argc, char **argv, Session *s)
{
	char *value;

	s = get_effective_session(c, s);

	if (argc < 2) {
		sock_answer_undefined(client_get_socket(c));
		return 0;
	}

	value = option_get(argv[1], c, s, OPTION_SCOPE_ANY);

	if (!value) {
		sock_answer_undefined(client_get_socket(c));
		return 0;
	} else {
		sock_printf(client_get_socket(c), "%s\n", value);
		free(value);
		return 0;
	}
}

static int
shutdown_handler(Client *c, int argc, char **argv, Session *s)
{
	if (argc != 2 || strcmp(argv[1], "IMMEDIATE")) {
		sock_printf(client_get_socket(c), "%s\n", SOCK_ANSWER_UNDEFINED);
		return 0;
	}

	if (!client_get_user(c)) {
		sock_answer_unavailable(client_get_socket(c));
		return 0;
	} else {
		exit_program(-1);
		return 0;  /* never reached */
	}
}

static int
who_handler(Client *c, int argc, char **argv, Session *s)
{
	char *who;

	/* additional arguments are ignored */

	who = NULL;

	if (c != NULL)
		who = client_get_identity(c);

	if (who != NULL) {
		sock_printf(client_get_socket(c), "%s\n", who);
		free(who);
	} else {
		sock_printf(client_get_socket(c), "%s\n", SOCK_ANSWER_UNDEFINED);
	}

	return 0;
}

static int
within_handler(Client *c, int argc, char **argv, Session *s)
{
	Session *session;
	CommandHandler function = NULL;

	if (argc < 4 || strcmp(argv[1], "SESSION")) {
		sock_printf(client_get_socket(c), "%s\n", SOCK_ANSWER_UNDEFINED);
		return 0;
	}

	session = sessions_find_by_id(client_get_sessions(c), session_string_to_id(argv[2]));
	if (!session) {
		sock_printf(client_get_socket(c), "%s\n", SOCK_ANSWER_UNDEFINED);
		return 0;
	}

	if (!command_is_embeddable(argv[3])) {
		sock_printf(client_get_socket(c), "%s\n", SOCK_ANSWER_UNDEFINED);
		return 0;
	}

	function = commands_get_handler(argv[3]);

	if (function != NULL) {
		return function(c, argc-3, argv+3, session);
	} else {
		sock_printf(client_get_socket(c), "%s\n", SOCK_ANSWER_UNDEFINED);
	}

	return 0;
}


static Session *
get_effective_session(Client *c, Session *s)
{
	if (!s)
		return client_get_default_session(c);
	else
		return s;
}
