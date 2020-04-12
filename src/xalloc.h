#ifndef XALLOC_H
#define XALLOC_H

#include <stdlib.h>

void *xmalloc(size_t size, const char *caller);
void *xcalloc(size_t nmemb, size_t size, const char *caller);
void *xrealloc(void *ptr, size_t size, const char *caller);

#endif /* XALLOC_H */
