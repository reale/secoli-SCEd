#ifndef SESSION_H
#define SESSION_H

#include "location.h"
#include "queue.h"
#include "search.h"
#include "utils.h"  /* bool */

typedef unsigned int session_id_t;

/* A session.  */
typedef struct Session {
	session_id_t id;

	Search *search;

	int cod_state;
	bool territorial_competence;

	int destroy;

	char *province;

} Session;


Session *session_create();

int session_destroy(Session *s);



char *session_get_identity(Session *s);
int session_verify_identity(Session *s, char *identity);

Session *sessions_add_session(Queue *sessionlist, Session *s);
//Session *sessions_remove_session(Queue *sessionlist, Session *s, QueueDirection direction);

Session *sessions_getfirst(Queue *sessionlist);
Session *sessions_getnext(Queue *sessionlist);
int sessions_session_count(Queue *sessionlist);

char* session_id_to_string(Session *s);
Session * sessions_find_by_id(Queue *sessionlist, session_id_t id);
session_id_t session_string_to_id(const char *str);
int session_create_search(Session *s);
int session_execute_search(Session *s);
int session_set_cod_state(Session *s, int cod_state);
bool session_get_territorial_competence(Session *s);
int session_set_territorial_competence(Session *s, bool value);
int session_refresh_search_options(Session *s, Search *search);
int session_count_search_results(Session *s);
Search *session_get_search(Session *s);
void session_destroy_search(Session *s);
int session_get_cod_state(Session *s);
char *session_get_province(Session *s);
int session_set_province(Session *s, const char *province);

#endif  /* SESSION_H */
