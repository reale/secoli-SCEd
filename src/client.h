#ifndef CLIENT_H
#define CLIENT_H

#include <uuid/uuid.h>

#include "encoder.h"
#include "queue.h"
#include "output.h"
#include "session.h"
#include "user.h"
#include "utils.h"   /* bool */


typedef struct OutputHandler OutputHandler;  /* Client and OutputHandler reference each other */

/* A client.  */
typedef struct Client {
	uuid_t uuid;
	int sock;
	bool destroy;

	Queue *messages;

	Queue *sessions;
	Session *default_session;

	Encoder *input_enc;
	Encoder *output_enc;

	OutputHandler *OH;

	char *field_delimiter;

	long int timeout;
	long int elapsed_time;
	
	User *user;

} Client;


/*
 * CLIENT methods
 */

Client *client_create(int sock);
int client_destroy(Client *c);

int client_add_message(Client *c, char *message);

char *client_get_message(Client *c);

char *client_get_identity(Client *c);

int client_verify_identity(Client *c, char *identity);


Session * client_get_default_session(Client *c); // XXX



void client_remove_session(Client *c, Session *s);

void client_set_destroy(Client *c, bool destroy);
bool client_get_destroy(Client *c);

/* timeout */
int client_check_timeout(Client *c, int t_diff);
int client_reset_elapsed_time(Client *c);

/* field delimiter */
char * client_get_field_delimiter(Client *c);
int client_set_field_delimiter(Client *c, const char *delimiter);

/* output encoder */
Encoder *client_get_output_enc(Client *c);
int client_set_output_enc(Client *c, Encoder *enc);
char *client_run_output_enc(Client *c, char *string);

/* output handler */
OutputHandler *client_get_output_handler(Client *c);
int client_set_output_handler(Client *c, OutputHandler *OH);
int client_run_output_handler(Client *c, Search *search, bool print_result_count);

/* session */
Session *client_get_session(Client *c);
int client_add_session(Client *c, Session *s);

/* socket */
int client_get_socket(Client *c);
int client_set_socket(Client *c, int sock);

/* timeout */
int client_get_timeout(Client *c);
int client_set_timeout(Client *c, int timeout);

/* user */
User *client_get_user(Client *c);
int client_set_user(Client *c, User *user);

Queue *client_get_sessions(Client *c);

/*
 * CLIENTS QUEUE methods
 */

int clients_init_all(void);
int clients_shutdown_all(void);
int clients_check_timeouts(int t_diff);

Client *clients_add_client(Client *c);
Client *clients_remove_client(Client *c, QueueDirection direction);

Client *clients_getfirst(void);
Client *clients_getnext(void);
int clients_client_count(void);

#endif  /* CLIENT_H */
