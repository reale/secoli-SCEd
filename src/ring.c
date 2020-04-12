#include <stdlib.h>
#include <string.h>
#include <vrb.h>

#include "ring.h"
#include "xalloc.h"


Ring *
ring_create(size_t size)
{
	Ring *buf;

	buf = xmalloc(sizeof(Ring), __FUNCTION__);
	if (!buf)
		return NULL;

	buf->vrb = vrb_new(size, NULL);
	if (!buf->vrb) {
		/* TODO: report */
		ring_destroy(buf);
		return NULL;
	}

	return buf;
}

void
ring_destroy(Ring *buf)
{
	if (!buf)
		return;

	if (buf->vrb)
		vrb_destroy(buf->vrb);

	free(buf);
}

void
ring_clear(Ring *buf)
{
	char *p;
	size_t size;

	if (!buf || !buf->vrb || vrb_is_empty(buf->vrb))
		return;
	
	p = vrb_data_ptr(buf->vrb);
	size = vrb_data_len(buf->vrb);

	memset(p, '\0', size);
}

size_t
ring_get_max_write(Ring *buf)
{
	if (!buf || !buf->vrb)
		return 0;
	
	return vrb_space_len(buf->vrb);
}

size_t
ring_get_max_read(Ring *buf)
{
	if (!buf || !buf->vrb)
		return 0;
	
	return vrb_data_len(buf->vrb);
}

int
ring_write(Ring *buf, char *source, size_t size)
{
	size_t written;

	if (!buf || !source || size <= 0)
		return -1;
	
	if (!buf->vrb)
		return -1;

	written = vrb_put(buf->vrb, source, size);

	if (written < size)
		return -1;
	else
		return 0;
}

size_t
ring_read(Ring *buf, char *target, size_t size)
{
	if (!buf || !target || size <= 0)
		return -1;

	if (!buf->vrb)
		return -1;

	return vrb_get(buf->vrb, target, size);
}

char *
ring_read_string(Ring *buf)
{
	char *p, *target;
	size_t n, size;

	if (!buf || !buf->vrb || vrb_is_empty(buf->vrb))
		return NULL;
	
	p = vrb_data_ptr(buf->vrb);
	n = vrb_data_len(buf->vrb);

	while (--n >= 0) {
		if (*p == '\r' || *p == '\n' || *p == '\0')
			break;
		p++;
	};

	if (n < 0)
		return NULL;

	size = vrb_data_len(buf->vrb) - n;
	target = xmalloc(size, __FUNCTION__);
	if (!target)
		return NULL;

	ring_read(buf, target, size);
	target[size - 1] = '\0';

	return target;
}
