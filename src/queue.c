#include <glib.h>

#include "queue.h"
#include "xalloc.h"


Queue *
queue_create(void)
{
	Queue *q = xmalloc(sizeof(Queue), __FUNCTION__);
	if (!q)
		return NULL;
	
	q->__queue = g_queue_new();
	q->__position = -1;
	
	return q;
}

void
queue_destroy(Queue *q)
{
	if (!q)
		return;
	
	g_queue_free(q->__queue);

	free(q);
}

int
queue_rewind(Queue *q)
{
	if (!q)
		return -1;
	
	if (q->__position > 0)
		q->__position = 0;
	
	return 0 ;

}

int
queue_end(Queue *q)
{
	if (!q)
		return -1;

	q->__position = queue_get_length(q) - 1;

	return 0;
}

int
queue_next(Queue *q)
{
	if (!q || q->__position >= queue_get_length(q) - 1)
		return -1;

	q->__position++;

	return 0;
}

int
queue_next_plus_one(Queue *q)
{
	if (!q)
		return -1;

	if (q->__position < queue_get_length(q))
		q->__position++;

	return 0;
}

int
queue_prev(Queue *q)
{
	if (!q)
		return -1;

	if (q->__position > 0)
		q->__position--;

	return 0;
}

void *
queue_get(Queue *q)
{
	if (!q || q->__position < 0)
		return NULL;

	return g_queue_peek_nth(q->__queue, q->__position);
}

int
queue_put(Queue *q, void *data)
{
	GList *link;

	if (!q || q->__position < 0)
		return -1;
	
	link = g_queue_peek_nth_link(q->__queue, q->__position);

	link->data = data;

	return 0;
}

void *
queue_get_first(Queue *q)
{
	return (!queue_rewind(q) ? queue_get(q) : NULL);
}

void *
queue_get_next(Queue *q)
{
	return (!queue_next(q) ? queue_get(q) : NULL);
}

void *
queue_get_prev(Queue *q)
{
	return (!queue_prev(q) ? queue_get(q) : NULL);
}

void *
queue_get_last(Queue *q)
{
	return (!queue_end(q) ? queue_get(q) : NULL);
}

int
queue_add_node(Queue *q, void *data)
{
	if (!queue_next_plus_one(q)) {
		g_queue_push_nth(q->__queue, data, q->__position);
		return 0;
	} else {
		return -1;
	}
}

int
queue_insert_node(Queue *q, void *data)
{
	if (!queue_prev(q)) {
		g_queue_push_nth(q->__queue, data, q->__position);
		return queue_next(q);
	} else {
		return -1;
	}
}

void *
queue_delete_node(Queue *q, QueueDirection direction)
{
	void *data;

	if (!q)
		return NULL;
	
	data = g_queue_pop_nth(q->__queue, q->__position);

	switch (direction) {
		case QD_HEAD:
			queue_rewind(q);
			break;
		case QD_TAIL:
			queue_end(q);
			break;
		case QD_PREV:
			queue_prev(q);
			break;
		case QD_NEXT:
		default:
			break;
	}

	return data;
}

void *
queue_remove(Queue *q, void *data, QueueDirection direction)
{
	void *node_data;

	if (!data || queue_rewind(q) < 0)
		return NULL;

	for (node_data = queue_get(q); node_data; node_data = queue_get_next(q)) {
		if (node_data == data)
			return queue_delete_node(q, direction);
	} while (queue_get_next(q));

	return NULL;
}

void *
queue_pop(Queue *q)
{
	return (!queue_end(q) ? queue_delete_node(q, QD_PREV) : NULL);
}

int
queue_push(Queue *q, void *data)
{
	if (!data)
		return -1;
	
	return (!queue_end(q) ? queue_add_node(q, data) : -1);
}

void *
queue_dequeue(Queue *q)
{
	return (!queue_rewind(q) ? queue_delete_node(q, QD_NEXT) : NULL);
}

int
queue_get_length(Queue *q)
{
	int length;

	if (!q)
		return -1;

	length = (int) g_queue_get_length(q->__queue);

	return (length >= 0 ? length : -1);
}

void *
queue_find(Queue *q, int (*compare)(void *, void *), void *data)
{
	void *node_data;

	if (!compare || !data)
		return NULL;
	
	for (node_data = queue_get(q); node_data; node_data = queue_get_next(q)) {
		if (compare(node_data, data) == 0)
			return data;
	}

	return NULL;
}

int
queue_get_position(Queue *q)
{
	if (!q || q->__position < 0)
		return -1;

	return q->__position;
}

void
queue_set_position(Queue *q, int position)
{
	if (q)
		q->__position = position;
}
