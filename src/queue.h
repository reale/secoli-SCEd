#ifndef QUEUE_H
#define QUEUE_H

#include <glib.h>



typedef enum QueueDirection {
	QD_HEAD, QD_PREV, QD_CURRENT, QD_NEXT, QD_TAIL
} QueueDirection;

typedef struct Queue {
	GQueue *__queue;
	int     __position;
} Queue;


Queue *queue_create(void);
void queue_destroy(Queue *q);

int queue_rewind(Queue *q);
int queue_end(Queue *q);
int queue_next(Queue *q);
int queue_next_plus_one(Queue *q);
int queue_prev(Queue *q);

void *queue_get(Queue *q);
int queue_put(Queue *q, void *data);

void *queue_get_first(Queue *q);
void *queue_get_next(Queue *q);
void *queue_get_prev(Queue *q);
void *queue_get_last(Queue *q);

int queue_add_node(Queue *q, void *data);
int queue_insert_node(Queue *q, void *data);
void *queue_delete_node(Queue *q, QueueDirection direction);
void *queue_remove(Queue *q, void *data, QueueDirection direction);

void *queue_pop(Queue *q);
int queue_push(Queue *q, void *data);
void *queue_dequeue(Queue *q);
#define  queue_enqueue  queue_push

int queue_get_length(Queue *q);

void *queue_find(Queue *q, int (*compare)(void *, void *), void *data);

int queue_get_position(Queue *q);
void queue_set_position(Queue *q, int position);

#endif  /* QUEUE_H */
