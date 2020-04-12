#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "location.h"
#include "main.h"
#include "queue.h"
#include "report.h"
#include "search.h"
#include "sce.h"
#include "session.h"
#include "utils.h"  /* bool */
#include "xalloc.h"


#define SESSION_ID_UNSET -1


static session_id_t session_counter = 0;

static session_id_t session_generate_id(void);


static session_id_t
session_generate_id(void)
{
	return session_counter++;
}

Session *
session_create()
{
	Session *s;

	debug(RPT_DEBUG, "%s()", __FUNCTION__);

	s = xmalloc(sizeof(Session), __FUNCTION__);
	if (!s)
		return NULL;

	s->id = SESSION_ID_UNSET;
	s->cod_state = COD_STATE_UNSET;
	s->province = NULL;
	s->territorial_competence = false;

	s->id = session_generate_id();

	s->destroy = 0; // XXX

	return s;
}

int
session_destroy(Session *s)
{
	if (!s)
		return -1;

	debug(RPT_DEBUG, "%s(id=[%d])", __FUNCTION__, s->id);

	if (s->province)
		free(s->province);

	free(s);

	debug(RPT_DEBUG, "%s: Session destroyed", __FUNCTION__);

	return 0;
}


int
session_create_search(Session *s)
{
	Search *search;

	if (!s)
		return -1;

	search = search_create(s->cod_state, NULL, SCE_CODE_UNSET, NULL, NULL);
	if (!search)
		return -1;
	
	s->search = search;

	return 0;
}

Session *
sessions_add_session(Queue *sessionlist, Session *s)
{
	if (!sessionlist)
		return NULL;

	if (queue_push(sessionlist, s) == 0)
		return s;

	return NULL;
}

#if 0
Session *
sessions_remove_client(Queue *sessionlist, Session *s, QueueDirection direction)
{
	Session *session;

	if (!sessionlist)
		return NULL;

	session = queue_remove(sessionlist, s, direction);
	
	return session;
}
#endif

Session *
sessions_getfirst(Queue *sessionlist)
{
	if (!sessionlist)
		return NULL;
	else
		return (Session *) queue_get_first(sessionlist);
}

Session *
sessions_getnext(Queue *sessionlist)
{
	if (!sessionlist)
		return NULL;
	else
		return (Session *) queue_get_next(sessionlist);
}

int
sessions_session_count(Queue *sessionlist)
{
	if (!sessionlist)
		return UNSET_INT;
	else
		return queue_get_length(sessionlist);
}

#define SESSION_ID_BUF_LEN 31
#define UNSET_SESSION_ID -1

char *
session_id_to_string(Session *s)
{
	char *buf;

	if (!s)
		return NULL;
	
	buf = xmalloc(SESSION_ID_BUF_LEN, __FUNCTION__);

	snprintf(buf, SESSION_ID_BUF_LEN, "%u", s->id);

	return buf;
}

static int
compare_ids(session_id_t id1, session_id_t id2)
{
	if (id1 == UNSET_SESSION_ID || id2 == UNSET_SESSION_ID || id1 != id2)
		return -1;

	return 0;
}

int
session_compare_id(void *s, void *idp)
{
	session_id_t id1, id2;
	
	if (!s || !idp)
		return -1;
	
	id1 = ((Session *) s)->id;
	id2 = *((session_id_t *) idp);

	return compare_ids(id1, id2);
}

Session *
sessions_find_by_id(Queue *sessionlist, session_id_t id)
{
	if (!sessionlist)
		return NULL;

	return (Session *) queue_find(sessionlist, session_compare_id, &id);
}


session_id_t
session_string_to_id(const char *str)
{
	if (!str)
		return UNSET_SESSION_ID;

	return atoi(str);
}


int
session_execute_search(Session *s)
{
	if (!s)
		return -1;

	return search_execute(s->search);
}

int
session_count_search_results(Session *s)
{
	if (!s)
		return -1;
	
	return search_count_results(s->search);
}

void session_flush_locations(Session *s) {} // XXX

int
session_get_cod_state(Session *s)
{
	if (!s)
		return COD_STATE_UNSET;
	
	return s->cod_state;
}

int session_set_cod_state(Session *s, int cod_state)
{
	if (!s)
		return -1;
	
	s->cod_state = cod_state;

	return 0;
}

char *
session_get_province(Session *s)
{
	if (!s)
		return NULL;
	
	return s->province;
}

int
session_set_province(Session *s, const char *province)
{
	if (!s)
		return -1;
	
	s->province = strdup(province);

	return 0;
}

bool
session_get_territorial_competence(Session *s)
{
	if (!s)
		return NULL;
	
	return s->territorial_competence;
}

int
session_set_territorial_competence(Session *s, bool value)
{
	if (!s)
		return -1;
	
	s->territorial_competence = value;

	return 0;
}

int
session_refresh_search_options(Session *s, Search *search)
{
	if (!s || !search)
		return -1;

	search_set_cod_state(search, s->cod_state);
	search_set_territorial_competence(search, s->territorial_competence);

	return 0;
}

void
session_destroy_search(Session *s)
{
	if (!s)
		return;
	
	search_destroy(s->search);

	s->search = NULL;
}

Search *session_get_search(Session *s)
{
	if (!s)
		return NULL;
	
	return s->search;
}
