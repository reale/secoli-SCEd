#include <stdio.h>
#include <string.h>

#include "consulate.h"
#include "xalloc.h"

Consulate *
consulate_create(void)
{
	Consulate *cons = xmalloc(sizeof(Consulate), __FUNCTION__);

	cons->mae_code = 0;
	return cons;
}

int
consulate_set_mae_code(Consulate *cons, const char *value)
{
	if (!cons || !value)
		return -1;
	
	cons->mae_code = atoi(value);

	return 0;
}

int
consulate_get_mae_code(Consulate *cons)
{
	if (!cons)
		return 0;
	
	return cons->mae_code;
}

void
consulate_destroy(Consulate *cons)
{
	if (!cons)
		return;

	free(cons);
}
