#ifndef RING_H
#define RING_H

#include <vrb.h>

typedef struct Ring_t {
	vrb_p vrb;
} Ring;

Ring *ring_create(size_t size);
void ring_destroy(Ring *buf);
void ring_clear(Ring *buf);

size_t ring_get_max_read(Ring *buf);
size_t ring_get_max_write(Ring *buf);

size_t ring_read(Ring *buf, char *target, size_t size);
int ring_write(Ring *buf, char *source, size_t size);

char *ring_read_string(Ring *buf);

#endif  /* RING_H */
