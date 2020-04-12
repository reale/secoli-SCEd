#ifndef COMMANDS_H
#define COMMANDS_H

#include "client.h"
#include "session.h"



typedef int (*CommandHandler) (Client *c, int argc, char **argv, Session *s);

typedef struct command_handler {
	char *keyword;
	CommandHandler function;
	int is_embeddable;
} command_handler;

CommandHandler commands_get_handler(const char *cmd);

#endif  /* COMMANDS_H */
