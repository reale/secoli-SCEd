#include <stdlib.h>

#include "report.h"
#include "xalloc.h"


void *
xmalloc(size_t size, const char *caller)
{
	void *res;

	res = malloc(size);

	if (!res) {
		if (!caller)
			report(RPT_ERR, "Error allocating %z bytes", size);  /* %z is used for size_t types */
		else
			report(RPT_ERR, "%s: Error allocating %z bytes", caller, size);
		return NULL;
	}

	return res;
}

void *
xcalloc(size_t nmemb, size_t size, const char *caller)
{
	void *res;

	res = calloc(nmemb, size);

	if (!res) {
		if (!caller)
			report(RPT_ERR, "Error allocating %z chunks of size %z each", nmemb, size);
		else
			report(RPT_ERR, "%s: Error allocating %z chunks of size %z each", caller, nmemb, size);
		return NULL;
	}

	return res;
}

void *
xrealloc(void *ptr, size_t size, const char *caller)
{
	void *res;

	res = realloc(ptr, size);

	if (!res) {
		if (!caller)
			report(RPT_ERR, "Error reallocating %z bytes", size);
		else
			report(RPT_ERR, "%s: Error reallocating %z bytes", caller, size);
		return NULL;
	}

	return res;
}
