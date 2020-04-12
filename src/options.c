#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>  /* mallinfo */

#include "identity.h"
//#include "journal.h"
#include "options.h"
#include "sce.h"
#include "search.h"
#include "utils.h"  /* bool */
#include "xalloc.h"
#include "xsqlite.h"


static int get_option_index(const char *cmd, int scope);
static inline bool parse_boolean(const char *string);

static char *client_identity_option_getter(Client *c, Session *s, int scope, bool within_list);
static int client_identity_option_setter(Client *c, const char *value, Session *s, int scope); 
static char *cod_state_option_getter(Client *c, Session *s, int scope, bool within_list);
static int cod_state_option_setter(Client *c, const char *value, Session *s, int scope); 
static char *elapsed_time_option_getter(Client *c, Session *s, int scope, bool within_list);
static int elapsed_time_option_setter(Client *c, const char *value, Session *s, int scope); 
static char *field_delimiter_option_getter(Client *c, Session *s, int scope, bool within_list);
static int field_delimiter_option_setter(Client *c, const char *value, Session *s, int scope); 
static char *logging_option_getter(Client *c, Session *s, int scope, bool within_list);
static int logging_option_setter(Client *c, const char *value, Session *s, int scope); 
static char *memory_consumption_option_getter(Client *c, Session *s, int scope, bool within_list);
static int memory_consumption_option_setter(Client *c, const char *value, Session *s, int scope); 
static char *output_enc_option_getter(Client *c, Session *s, int scope, bool within_list);
static int output_enc_option_setter(Client *c, const char *value, Session *s, int scope); 
static char *output_handler_option_getter(Client *c, Session *s, int scope, bool within_list);
static int output_handler_option_setter(Client *c, const char *value, Session *s, int scope); 
static char *province_option_getter(Client *c, Session *s, int scope, bool within_list);
static int province_option_setter(Client *c, const char *value, Session *s, int scope); 
static char *search_constraint_option_getter(Client *c, Session *s, int scope, bool within_list);
static int search_constraint_option_setter(Client *c, const char *value, Session *s, int scope); 
static char *server_identity_option_getter(Client *c, Session *s, int scope, bool within_list);
static int server_identity_option_setter(Client *c, const char *value, Session *s, int scope); 
static char *territorial_competence_option_getter(Client *c, Session *s, int scope, bool within_list);
static int territorial_competence_option_setter(Client *c, const char *value, Session *s, int scope); 
static char *timeout_option_getter(Client *c, Session *s, int scope, bool within_list);
static int timeout_option_setter(Client *c, const char *value, Session *s, int scope); 


static option_handler options[] = {
	{
		"CLIENT_IDENTITY",
		client_identity_option_getter,
		client_identity_option_setter,
		OPTION_SCOPE_CLIENT,
		OPTION_RESTRICTION_NONE
	},
	{
		"COD_STATE",
		cod_state_option_getter,
		cod_state_option_setter,
		OPTION_SCOPE_SEARCH | OPTION_SCOPE_SESSION,
		OPTION_RESTRICTION_NONE
	},
	{
		"ELAPSED_TIME",
		elapsed_time_option_getter,
		elapsed_time_option_setter,
		OPTION_SCOPE_ANY,
		OPTION_RESTRICTION_NONE
	},
	{
		"FIELD_DELIMITER",
		field_delimiter_option_getter,
		field_delimiter_option_setter,
		OPTION_SCOPE_CLIENT,
		OPTION_RESTRICTION_NONE
	},
	{
		"LOGGING",
		logging_option_getter,
		logging_option_setter,
		OPTION_SCOPE_CLIENT,
		OPTION_RESTRICTION_READING | OPTION_RESTRICTION_WRITING
	},
	{
		"MEMORY_CONSUMPTION",
		memory_consumption_option_getter,
		memory_consumption_option_setter,
		OPTION_SCOPE_ANY,
		OPTION_RESTRICTION_NONE
	},
	{
		"OUTPUT_ENC",
		output_enc_option_getter,
		output_enc_option_setter,
		OPTION_SCOPE_CLIENT,
		OPTION_RESTRICTION_NONE
	},
	{
		"OUTPUT_HANDLER",
		output_handler_option_getter,
		output_handler_option_setter,
		OPTION_SCOPE_CLIENT,
		OPTION_RESTRICTION_NONE
	},
	{
		"PROVINCE",
		province_option_getter,
		province_option_setter,
		OPTION_SCOPE_SEARCH | OPTION_SCOPE_SESSION,
		OPTION_RESTRICTION_NONE
	},
	{
		"SEARCH_CONSTRAINT",
		search_constraint_option_getter,
		search_constraint_option_setter,
		OPTION_SCOPE_SEARCH | OPTION_SCOPE_SESSION,
		OPTION_RESTRICTION_NONE
	},
	{
		"SERVER_IDENTITY",
		server_identity_option_getter,
		server_identity_option_setter,
		OPTION_SCOPE_CLIENT,
		OPTION_RESTRICTION_NONE
	},
	{
		"TERRITORIAL_COMPETENCE",
		territorial_competence_option_getter,
		territorial_competence_option_setter,
		OPTION_SCOPE_SEARCH | OPTION_SCOPE_SESSION,
		OPTION_RESTRICTION_NONE
	},
	{
		"TIMEOUT",
		timeout_option_getter,
		timeout_option_setter,
		OPTION_SCOPE_CLIENT,
		OPTION_RESTRICTION_NONE
	},
	{
		NULL, NULL, NULL, OPTION_SCOPE_UNSET, OPTION_RESTRICTION_NONE
	}
};

static int
get_option_index(const char *cmd, int scope)
{
	int i;

	if (!cmd)
		return -1;

	for (i = 0; options[i].keyword != NULL; i++) {
		if (strcmp(cmd, options[i].keyword) == 0 && (scope & options[i].scope)) {
			return i;
		}
	}

	return -1;
}

OptionGetter
options_get_getter(const char *cmd, int scope)
{
	int i = get_option_index(cmd, scope);

	if (i < 0)
		return NULL;
	else
		return options[i].getter;
}

OptionSetter
options_get_setter(const char *cmd, int scope)
{
	int i = get_option_index(cmd, scope);

	if (i < 0)
		return NULL;
	else
		return options[i].setter;
}

char *
option_get(const char *cmd, Client *c, Session *s, int scope)
{
	OptionGetter getter;
	int i;
	
	if (!cmd)
		return NULL;

	i = get_option_index(cmd, scope);
	if (i < 0)
		return NULL;

	if (options[i].restriction & OPTION_RESTRICTION_READING && !client_get_user(c))
		return NULL;

	getter = options[i].getter;
	if (!getter)
		return NULL;
	
	return getter(c, s, scope, false);
}

int
option_set(const char *cmd, const char *value, Client *c, Session *s, int scope)
{
	OptionSetter setter;
	int i;
	
	if (!cmd || !value)
		return -1;

	i = get_option_index(cmd, scope);
	if (i < 0)
		return -1;

	if (options[i].restriction & OPTION_RESTRICTION_WRITING && !client_get_user(c))
		return -1;

	setter = options[i].setter;
	if (!setter)
		return -1;
	
	if (setter(c, value, s, scope) < 0)
		return -1;
	
	return 0;
}

char *
options_list(Client *c, Session *s, int scope, int index)
{
	OptionGetter getter;
	char *value;
	char *buffer = NULL;
	size_t buflen;

	if (options[index].keyword != NULL) {
		getter = options[index].getter;
		
		if (getter) {
			value = getter(c, s, scope, true);

			if (!value)
				value = strdup("NULL");

			buflen = strlen(options[index].keyword) + 1 + strlen(value) + 3;
			buffer = xmalloc(buflen, __FUNCTION__);

			if (scope & options[index].scope)
				snprintf(buffer, buflen-2, "%s %s", options[index].keyword, value);
			else
				/* options in another scope are enclosed within square brackets */
				snprintf(buffer, buflen, "%s [%s]", options[index].keyword, value);

			free(value);
		}
	}

	return buffer;
}

static inline bool
parse_boolean(const char *string)
{
	return (!strcmp(string, "TRUE"));
}


static char *
client_identity_option_getter(Client *c, Session *s, int scope, bool within_list)
{
	/* equivalent to the "WHO" command */

	char *who = NULL;

	if (c != NULL)
		who = client_get_identity(c);
	
	return who;
}

static int
client_identity_option_setter(Client *c, const char *value, Session *s, int scope)
{
	return -1;
}

static char *
cod_state_option_getter(Client *c, Session *s, int scope, bool within_list)
{
	int cod_state = COD_STATE_UNSET;
	char *value;

	if (!c || !s)
		return NULL;

	if (scope & OPTION_SCOPE_SESSION) {
		cod_state = session_get_cod_state(s);
	} else if (scope & OPTION_SCOPE_SEARCH) {
		Search *search = session_get_search(s);
		if (!search)
			return NULL;
		cod_state = search_get_cod_state(search);
	} else {
		return NULL;
	}

	if (cod_state == COD_STATE_UNSET)
		return NULL;

	value = xmalloc(4, __FUNCTION__);  /* cod_state has at most 3 digits */
	snprintf(value, 4, "%d", cod_state);
	return value;
}

static int
cod_state_option_setter(Client *c, const char *value, Session *s, int scope)
{
	int cod_state;

	if (!c || !s || !value)
		return -1;

	cod_state = sce_validate_cod_state(value);
	
	if (cod_state == COD_STATE_UNSET)
		return -1;

	if (scope & OPTION_SCOPE_SESSION) {
		return session_set_cod_state(s, cod_state);
	} else if (scope & OPTION_SCOPE_SEARCH) {
		Search *search = session_get_search(s);
		if (!search)
			return -1;
		return search_set_cod_state(search, cod_state);
	} else {
		return -1;
	}
}

static char *
elapsed_time_option_getter(Client *c, Session *s, int scope, bool within_list)
{
	char *buffer;

	if (!c)
		return NULL;

	buffer = xmalloc(16, __FUNCTION__);
	if (!buffer)
		return NULL;

	snprintf(buffer, 16, "%lu", c->elapsed_time);

	return buffer;
}

static int
elapsed_time_option_setter(Client *c, const char *value, Session *s, int scope)
{
	return -1;
}

static char *
field_delimiter_option_getter(Client *c, Session *s, int scope, bool within_list)
{
	char *delimiter;

	if (!c)
		return NULL;
	
	delimiter = client_get_field_delimiter(c);
	if (!delimiter)
		return NULL;

	return strdup(delimiter);
}

static int
field_delimiter_option_setter(Client *c, const char *value, Session *s, int scope)
{
	if (!c || !value)
		return -1;
	
	return client_set_field_delimiter(c, value);
}

static char *
logging_option_getter(Client *c, Session *s, int scope, bool within_list)
{
	//return (bool2desc(journal_is_logging_enabled()));
	return NULL;  // XXX
}

static int
logging_option_setter(Client *c, const char *value, Session *s, int scope)
{
	return -1;  // XXX

	if (!value)
		return -1;
	
#if 0
	if (!strcmp(value, "ENABLE"))
		return journal_enable_logging();
	else if (!strcmp(value, "DISABLE"))
		return journal_disable_logging();
	else
		return -1;
#endif
}

static char *
memory_consumption_option_getter(Client *c, Session *s, int scope, bool within_list)
{
	char *buffer;
	struct mallinfo malloc_stats;

	if (!c)
		return NULL;

	if (within_list || !client_get_user(c)) {

		buffer = xmalloc(128, __FUNCTION__);
		if (!buffer)
			return NULL;

		malloc_stats = mallinfo();

		/* TODO:  should we add up memory used by other components?  */

		snprintf(buffer, 128, "%d", malloc_stats.arena);

	} else {
	
		/* for authenticated users and outside options list, print out in-depth info */

		memory_status memory_status;

		buffer = xmalloc(4096, __FUNCTION__);
		if (!buffer)
			return NULL;

		if (get_memory_status(&memory_status) < 0) {
			free(buffer);
			return NULL;
		}

		malloc_stats = mallinfo();

		snprintf(buffer, 4096,
			"MALLOC_ARENA %d\n"
			"TOTAL_PROGRAM_SIZE %lu\n"
			"RESIDENT_SET_SIZE %lu\n"
			"SHARED_PAGES %lu\n"
			"TEXT_SIZE %lu\n"
			"DATA_AND_STACK %lu\n"
			"SQLITE_USED_MEMORY %d",
			malloc_stats.arena,
			memory_status.size,
			memory_status.resident,
			memory_status.shared,
			memory_status.text,
			memory_status.data,
			xsqlite_get_memory_used());

	}

	return buffer;
}

static int
memory_consumption_option_setter(Client *c, const char *value, Session *s, int scope)
{
	return -1;
}

static char *
output_enc_option_getter(Client *c, Session *s, int scope, bool within_list)
{
	Encoder *enc;
	char *charset;

	if (!c)
		return NULL;
	
	enc = client_get_output_enc(c);
	if (!enc)
		return NULL;

	charset = encoder_get_output_charset(enc);
	if (!charset)
		return NULL;
	
	return strdup(charset);
}

static int
output_enc_option_setter(Client *c, const char *value, Session *s, int scope)
{
	Encoder *enc;

	if (!c || !value)
		return -1;
	
	enc = client_get_output_enc(c);
	if (!enc)
		return -1;

	enc = encoder_set_output_charset(enc, value);
	if (!enc)
		return -1;
	
	return client_set_output_enc(c, enc);
}

static char *
output_handler_option_getter(Client *c, Session *s, int scope, bool within_list)
{
	OutputHandler *OH;
	char *description;

	if (!c)
		return NULL;
	
	OH = client_get_output_handler(c);
	if (!OH)
		return NULL;

	description = output_get_handler_description(OH);
	if (!description)
		return NULL;
	
	return strdup(description);
}

static int
output_handler_option_setter(Client *c, const char *value, Session *s, int scope)
{
	OutputHandler *OH, *newOH;

	if (!c || !value)
		return -1;
	
	OH = client_get_output_handler(c);
	if (!OH)
		return -1;

	newOH = output_create_handler(c, value);
	if (!newOH)
		return -1;

	if (output_close_handler(OH) < 0) {
		/* if we cannot close the already existing handler,
		   let's destroy the newly-created one and give up */
		output_destroy_handler(newOH);
		return -1;
	}
	
	output_destroy_handler(OH);
	
	return client_set_output_handler(c, newOH);
}

static char *
province_option_getter(Client *c, Session *s, int scope, bool within_list)
{
	char *province = NULL;

	if (!c || !s)
		return NULL;

	if (scope & OPTION_SCOPE_SESSION) {
		province = session_get_province(s);
	} else if (scope & OPTION_SCOPE_SEARCH) {
		Search *search = session_get_search(s);
		if (!search)
			return NULL;
		province = search_get_province(search);
	} else {
		return NULL;
	}

	if (!province)
		return NULL;
	else
		return strdup(province);
}

static int
province_option_setter(Client *c, const char *value, Session *s, int scope)
{
	if (!c || !s || !value)
		return -1;

	if (scope & OPTION_SCOPE_SESSION) {
		return session_set_province(s, value);
	} else if (scope & OPTION_SCOPE_SEARCH) {
		Search *search = session_get_search(s);
		if (!search)
			return -1;
		return search_set_province(search, value);
	} else {
		return -1;
	}
}

static char *
search_constraint_option_getter(Client *c, Session *s, int scope, bool within_list)
{
	#if 0
	OutputHandler *OH;

	if (!c)
		return NULL;
	
	OH = client_get_search_constraint(c);
	if (!OH)
		return NULL;

	return output_get_handler_description(OH);
	#endif
	return NULL;
}

static int
search_constraint_option_setter(Client *c, const char *value, Session *s, int scope)
{
	#if 0
	OutputHandler *OH, *newOH;

	if (!c || !value)
		return -1;
	
	OH = client_get_search_constraint(c);
	if (!OH)
		return -1;

	newOH = output_create_handler(c, value);
	if (!newOH)
		return -1;

	if (output_close_handler(OH) < 0) {
		/* if we cannot close the already existing handler,
		   let's destroy the newly-created one and give up */
		output_destroy_handler(newOH);
		return -1;
	}
	
	output_destroy_handler(OH);
	
	return client_set_search_constraint(c, newOH);
	#endif
	return 0;
}

static char *
server_identity_option_getter(Client *c, Session *s, int scope, bool within_list)
{
	char *identity = NULL;

	if (c != NULL)
		identity = identity_get();
	
	return identity;
}

static int
server_identity_option_setter(Client *c, const char *value, Session *s, int scope)
{
	return -1;
}

static char *
territorial_competence_option_getter(Client *c, Session *s, int scope, bool within_list)
{
	bool territorial_competence;

	if (!c || !s)
		return NULL;
	
	if (scope & OPTION_SCOPE_SESSION) {
		territorial_competence = session_get_territorial_competence(s);
	} else if (scope & OPTION_SCOPE_SEARCH) {
		Search *search = session_get_search(s);
		if (!search)
			return NULL;
		territorial_competence = search_get_territorial_competence(s);
	} else {
		return NULL;
	}

	return boolean_to_string(territorial_competence);
}

static int
territorial_competence_option_setter(Client *c, const char *value, Session *s, int scope)
{
	bool territorial_competence;

	if (!c || !s || !value)
		return -1;

	territorial_competence = string_to_boolean(value);
	if (!territorial_competence)
		return -1;

	if (scope & OPTION_SCOPE_SESSION) {
		return session_set_territorial_competence(s, territorial_competence);
	} else if (scope & OPTION_SCOPE_SEARCH) {
		Search *search = session_get_search(s);
		if (!search)
			return -1;
		return search_set_territorial_competence(search, territorial_competence);
	} else {
		return -1;
	}
}

static char *
timeout_option_getter(Client *c, Session *s, int scope, bool within_list)
{
	char *buffer;
	int timeout;

	if (!c)
		return NULL;

	buffer = xmalloc(16, __FUNCTION__);
	if (!buffer)
		return NULL;

	if (scope & OPTION_SCOPE_CLIENT) {
		timeout = client_get_timeout(c);
		snprintf(buffer, 16, "%d", timeout);
		return buffer;
	} else {
		return NULL;
	}
}

static int
timeout_option_setter(Client *c, const char *value, Session *s, int scope)
{
	int timeout;

	if (!c || !value)
		return -1;

	if (scope & OPTION_SCOPE_CLIENT) {
		timeout = atoi(value);
		return client_set_timeout(c, timeout);
	} else {
		return -1;
	}
}
