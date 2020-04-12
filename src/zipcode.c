#include <stdio.h>
#include <string.h>

#include "sce.h"  /* COD_STATE_UNSET */
#include "xalloc.h"
#include "zipcode.h"


Zipcode *
zipcode_create(void)
{
	Zipcode *zipcode = xmalloc(sizeof(Zipcode), __FUNCTION__);

	zipcode->cod_state = COD_STATE_UNSET;
	zipcode->code = NULL;
	return zipcode;
}

int
zipcode_set_cod_state(Zipcode *zipcode, const char *value)
{
	if (!zipcode || !value)
		return -1;
	
	zipcode->cod_state = atoi(value);

	return 0;
}

int
zipcode_set_cod_state_integer(Zipcode *zipcode, int value)
{
	if (!zipcode || value == COD_STATE_UNSET)
		return -1;
	
	zipcode->cod_state = value;

	return 0;
}

int
zipcode_get_cod_state(Zipcode *zipcode)
{
	if (!zipcode)
		return 0;
	
	return zipcode->cod_state;
}

int
zipcode_set_code(Zipcode *zipcode, const char *value)
{
	if (!zipcode || !value)
		return -1;
	
	zipcode->code = strdup(value);

	return 0;
}

const char *
zipcode_get_code(Zipcode *zipcode)
{
	if (!zipcode)
		return 0;
	
	return zipcode->code;
}

void
zipcode_destroy(Zipcode *zipcode)
{
	if (!zipcode)
		return;
	
	if (zipcode->code)
		free(zipcode->code);

	free(zipcode);
}
