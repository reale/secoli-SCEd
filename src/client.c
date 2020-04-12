#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <uuid/uuid.h>

#include "client.h"
#include "encoder.h"
#include "main.h"
#include "report.h"
#include "queue.h"
#include "sock.h"
#include "xalloc.h"
#include "output.h"
#include "output_csv.h"  /* OUTPUT_CSV_DEFAULT_DELIMITER */
#include "user.h"
#include "utils.h"  /* bool */


/* CLIENTS QUEUE */

Queue *clients_queue = NULL;


/*
 * CLIENT methods
 */

Client *
client_create(int sock)
{
	Client *c;

	/* Client */

	debug(RPT_DEBUG, "%s(sock=%i)", __FUNCTION__, sock);

	c = (Client *) xmalloc(sizeof(Client), __FUNCTION__);
	if (!c)
		return NULL;

	c->sock = SOCK_UNSET;

	/* Message queue */

	debug(RPT_DEBUG, "%s(sock=%i): Creating message queue", __FUNCTION__, sock);

	c->messages = queue_create();
	if (!c->messages) {
		report(RPT_ERR, "%s(sock=%i): Error creating message queue", __FUNCTION__, sock);
		client_destroy(c);
		return NULL;
	}

	/* Session queue */

	debug(RPT_DEBUG, "%s(sock=%i): Creating session queue", __FUNCTION__, sock);

	c->sessions = queue_create();
	if (!c->sessions) {
		report(RPT_ERR, "%s(sock=%i): Error creating session list", __FUNCTION__, sock);
		client_destroy(c);
		return NULL;
	}

	/* Default session */

	debug(RPT_DEBUG, "%s(sock=%i): Creating default session", __FUNCTION__, sock);

	c->default_session = session_create();
	if (!c->default_session) {
		report(RPT_ERR, "%s(sock=%i): Error creating default session", __FUNCTION__, sock);
		client_destroy(c);
		return NULL;
	}

	/* Input encoder */

	debug(RPT_DEBUG, "%s(sock=%i): Creating input encoder", __FUNCTION__, sock);

	c->input_enc = encoder_create(ENCODER_DEFAULT_CHARSET, ENCODER_DIRECTION_INPUT);
	if (!c->input_enc) {
		report(RPT_ERR, "%s(sock=%i): Error creating the input encoder", __FUNCTION__, sock);
		client_destroy(c);
		return NULL;
	}

	/* Output encoder */

	debug(RPT_DEBUG, "%s(sock=%i): Creating output encoder", __FUNCTION__, sock);

	c->output_enc = encoder_create(ENCODER_DEFAULT_CHARSET, ENCODER_DIRECTION_OUTPUT);
	if (!c->output_enc) {
		report(RPT_ERR, "%s(sock=%i): Error creating the output encoder", __FUNCTION__, sock);
		client_destroy(c);
		return NULL;
	}

	/* Output handler */

	debug(RPT_DEBUG, "%s(sock=%i): Creating output handler", __FUNCTION__, sock);

	c->OH = output_create_handler(c, OUTPUT_DEFAULT_HANDLER);
	if (!c->OH) {
		report(RPT_ERR, "%s(sock=%i): Error creating the output handler", __FUNCTION__, sock);
		client_destroy(c);
		return NULL;
	}

	debug(RPT_DEBUG, "%s(sock=%i): Setting output field delimiter to %s", __FUNCTION__, sock, OUTPUT_CSV_DEFAULT_DELIMITER);
	c->field_delimiter = strdup(OUTPUT_CSV_DEFAULT_DELIMITER);

	/* UUID */

	debug(RPT_DEBUG, "%s(sock=%i): Generating client ID", __FUNCTION__, sock);
	uuid_generate(c->uuid);

	/* Complete instantiation */

	c->user = NULL;
	c->timeout = 0;
	c->elapsed_time = 0;
	c->destroy = 0;
	c->sock = sock;

	return c;
}

int
client_check_timeout(Client *c, int t_diff)
{
	if (!c)
		return -1;

	c->elapsed_time += t_diff;
	if (c->timeout > 0 && c->elapsed_time >= c->timeout)
		client_set_destroy(c, 1);
	return 0;
}

int
client_reset_elapsed_time(Client *c)
{
	if (!c)
		return -1;
	
	c->elapsed_time = 0;
	return 0;
}

int
client_destroy(Client *c)
{
	char *str;
	Session *s;

	if (!c)
		return -1;

	debug(RPT_DEBUG, "%s(c=[%d])", __FUNCTION__, c->sock);

	debug(RPT_DEBUG, "%s(sock=%i): Closing the socket", __FUNCTION__, c->sock);
	sock_close(c->sock);

	debug(RPT_DEBUG, "%s(sock=%i): Discarding all user messages", __FUNCTION__, c->sock);
	while ((str = client_get_message(c)))
		free(str);
	queue_destroy(c->messages);

	debug(RPT_DEBUG, "%s(sock=%i): Destroying all sessions", __FUNCTION__, c->sock);
	while ((s = client_get_session(c)))
		session_destroy(s);
	queue_destroy(c->sessions);

	debug(RPT_DEBUG, "%s(sock=%i): Destroying default session", __FUNCTION__, c->sock);
	session_destroy(c->default_session);

	debug(RPT_DEBUG, "%s(sock=%i): Destroying encoders", __FUNCTION__, c->sock);
	encoder_destroy(c->input_enc);
	encoder_destroy(c->output_enc);

	debug(RPT_DEBUG, "%s(sock=%i): Destroying handlers", __FUNCTION__, c->sock);
	output_destroy_handler(c->OH);

	debug(RPT_DEBUG, "%s(sock=%i): Destroying field delimiter", __FUNCTION__, c->sock);
	if (c->field_delimiter)
		free(c->field_delimiter);

	debug(RPT_DEBUG, "%s(sock=%i): Destroying client ID", __FUNCTION__, c->sock);
	uuid_clear(c->uuid);

	debug(RPT_DEBUG, "%s(sock=%i): Destroying user", __FUNCTION__, c->sock);
	if (c->user)
		user_destroy(c->user);

	debug(RPT_DEBUG, "%s(sock=%i): Destroying client...", __FUNCTION__, c->sock);
	free(c);

	debug(RPT_DEBUG, "%s: Client data removed", __FUNCTION__);

	return 0;
}


int
client_add_message(Client *c, char *message)
{
	int err = 0;

	if (!c || !message)
		return -1;

	if (strlen(message) > 0) {
		debug(RPT_DEBUG, "%s(c=[%d], message=\"%s\")", __FUNCTION__,
			c->sock, message);
		queue_enqueue(c->messages, (void *) message);
	}

	return err;
}

char *
client_get_message(Client *c)
{
	char *str;

	debug(RPT_DEBUG, "%s(c=[%d])", __FUNCTION__, c->sock);

	if (!c)
		return NULL;

	str = (char *) queue_dequeue(c->messages);

	return str;
}

char *
client_get_identity(Client *c)
{
	char *uuid_unparsed;

	if (!c)
		return NULL;
	
	uuid_unparsed = (char *) xmalloc(37, __FUNCTION__);  /* 36 + '\0' */

	uuid_unparse_upper(c->uuid, uuid_unparsed);

	return uuid_unparsed;
}

int
client_verify_identity(Client *c, char *identity)
{
	uuid_t uuid_parsed;

	if (!c)
		return -1;

	if (uuid_parse(identity, uuid_parsed) != 0)
		return -1;

	if (uuid_compare(c->uuid, uuid_parsed) != 0)
		return -1;
	
	return 0;
}

Session *
client_get_default_session(Client *c)
{
	if (!c)
		return NULL;
	
	return c->default_session;
}


void
client_remove_session(Client *c, Session *s)
{
}

bool
client_get_destroy(Client *c)
{
	if (!c)
		return 0;

	return c->destroy;
}

void
client_set_destroy(Client *c, bool destroy)
{

	if (!c)
		return;

	c->destroy = destroy;
}

char *
client_get_field_delimiter(Client *c)
{
	if (!c)
		return NULL;
	else
		return c->field_delimiter;
}

int
client_set_field_delimiter(Client *c, const char *delimiter)
{
	if (!c)
		return -1;

	if (c->field_delimiter)
		free(c->field_delimiter);

	if (!delimiter)
		c->field_delimiter = NULL;
	else
		c->field_delimiter = strdup(delimiter);
	
	return 0;
}

Encoder *
client_get_output_enc(Client *c)
{
	if (!c)
		return NULL;
	
	return c->output_enc;
}

int
client_set_output_enc(Client *c, Encoder *enc)
{
	if (!c || !enc)
		return -1;
	
	c->output_enc = enc;
	
	return 0;
}

char *
client_run_output_enc(Client *c, char *string)
{
	if (!c || !string)
		return NULL;
	
	if (!c->output_enc)
		return NULL;
	
	return encoder_run(c->output_enc, string);
}

OutputHandler *
client_get_output_handler(Client *c)
{
	if (!c)
		return NULL;
	
	return c->OH;
}

int
client_set_output_handler(Client *c, OutputHandler *OH)
{
	if (!c || !OH)
		return -1;
	
	c->OH = OH;

	return 0;
}

int
client_run_output_handler(Client *c, Search *search, bool print_result_count)
{
	OutputHandler *OH;

	if (!c || !search)
		return -1;
	
	OH = client_get_output_handler(c);
	if (!OH)
		return -1;

	return output_run_handler(OH, search, print_result_count);
}

Queue *
client_get_sessions(Client *c)
{
	if (!c)
		return NULL;

	return c->sessions;
}

Session *
client_get_session(Client *c)
{
	if (!c)
		return NULL;

	return (Session *) queue_dequeue(c->sessions);
}

int
client_add_session(Client *c, Session *s)
{
	if (!c || !s)
		return -1;
	
	return queue_push(c->sessions, s);
}

int
client_get_socket(Client *c)
{
	if (!c)
		return SOCK_UNSET;
	
	return c->sock;
}

int
client_set_socket(Client *c, int sock)
{
	if (!c)
		return -1;
	
	c->sock = sock;

	return 0;
}

int
client_get_timeout(Client *c)
{
	if (!c)
		return 0;
	
	return c->timeout / 1e6;
}

int
client_set_timeout(Client *c, int timeout)
{
	int timeout2;

	if (!c)
		return -1;
	
	if (timeout >= (LONG_MAX / 1e6) || timeout < 0)
		timeout2 = LONG_MAX;
	else
		timeout2 = timeout * 1e6;
	
	if (client_get_user(c))
		c->timeout = timeout2;
	else if (c->timeout == 0 || (timeout2 > 0 && timeout2 < c->timeout))
		c->timeout = timeout2;

	return 0;
}

User *
client_get_user(Client *c)
{
	if (!c)
		return NULL;
	
	return c->user;
}

int
client_set_user(Client *c, User *user)
{
	if (!c)
		return -1;
	
	c->user = user;

	return 0;
}


/*
 * CLIENTS QUEUE methods
 */

int
clients_init_all(void)
{
	debug(RPT_DEBUG, "%s()", __FUNCTION__);

	clients_queue = queue_create();
	if (!clients_queue) {
		report(RPT_ERR, "%s: Unable to create client list", __FUNCTION__);
		return -1;
	}

	return 0;
}

int
clients_shutdown_all(void)
{
	Client *c;

	debug(RPT_DEBUG, "%s()", __FUNCTION__);

	if (!clients_queue)
		return -1;

	for (c = queue_get_first(clients_queue); c; c = queue_get_next(clients_queue)) {
		if (c) {
			if (client_destroy(c) != 0) {
				report(RPT_ERR, "%s: Error freeing client", __FUNCTION__);
			} else {
				debug(RPT_DEBUG, "%s: Freed client...", __FUNCTION__);
			}
		} else {
			debug(RPT_DEBUG, "%s: No clients!", __FUNCTION__);
		}
	}

	queue_destroy(clients_queue);

	debug(RPT_DEBUG, "%s: done", __FUNCTION__);

	return 0;
}

int
clients_check_timeouts(int t_diff)
{
	Client *c;

	if (!clients_queue)
		return -1;

	for (c = queue_get_first(clients_queue); c; c = queue_get_next(clients_queue)) {
		if (c) {
			client_check_timeout(c, t_diff);
			if (client_get_destroy(c))
				sock_destroy_client_socket(c);
		} else {
			debug(RPT_DEBUG, "%s: No clients!", __FUNCTION__);
		}
	}

	return 0;
}

Client *
clients_add_client(Client *c)
{
	if (queue_push(clients_queue, c) == 0)
		return c;

	return NULL;
}

Client *
clients_remove_client(Client *c, QueueDirection direction)
{
	Client *client = queue_remove(clients_queue, c, direction);
	
	return client;
}

Client *
clients_getfirst(void)
{
	return (Client *) queue_get_first(clients_queue);
}

Client *
clients_getnext(void)
{
	return (Client *) queue_get_next(clients_queue);
}

int
clients_client_count(void)
{
	return queue_get_length(clients_queue);
}
