#ifndef OPTIONS_H
#define OPTIONS_H

#include "client.h"
#include "session.h"
#include "utils.h"  /* bool */


#define OPTION_SCOPE_UNSET    0x0
#define OPTION_SCOPE_SEARCH   0x1
#define OPTION_SCOPE_SESSION  0x2
#define OPTION_SCOPE_CLIENT   0x4
#define OPTION_SCOPE_GLOBAL   0x8
#define OPTION_SCOPE_ANY      0xFF

#define OPTION_RESTRICTION_NONE     0x0
#define OPTION_RESTRICTION_WRITING  0x1
#define OPTION_RESTRICTION_READING  0x2


typedef char * (*OptionGetter) (Client *c, Session *s, int scope, bool within_list);
typedef int (*OptionSetter) (Client *c, const char *value, Session *s, int scope);

typedef struct option_handler {
	char *keyword;
	OptionGetter getter;
	OptionSetter setter;
	int scope;
	int restriction;
} option_handler;

char *options_list(Client *c, Session *s, int scope, int index);
OptionGetter options_get_getter(const char *cmd, int scope);
OptionSetter options_get_setter(const char *cmd, int scope);

char *option_get(const char *cmd, Client *c, Session *s, int scope);
int option_set(const char *cmd, const char *value, Client *c, Session *s, int scope);

#endif  /* OPTIONS_H */
