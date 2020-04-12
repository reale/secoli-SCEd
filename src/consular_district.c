#include <stdio.h>
#include <string.h>

#include "consular_district.h"
#include "search.h"  /* *_UNSET */
#include "xalloc.h"


ConsularDistrict *
consular_district_create(void)
{
	ConsularDistrict *cons = xmalloc(sizeof(ConsularDistrict), __FUNCTION__);

	cons->mae_code = MAE_CODE_UNSET;
	cons->territory_name = NULL;
	cons->sce_code = SCE_CODE_UNSET;
	return cons;
}

int
consular_district_set_mae_code(ConsularDistrict *cons, const char *value)
{
	if (!cons || !value)
		return -1;
	
	cons->mae_code = atoi(value);

	return 0;
}

int
consular_district_set_mae_code_int(ConsularDistrict *cons, int value)
{
	if (!cons)
		return -1;
	
	cons->mae_code = value;

	return 0;
}

int
consular_district_get_mae_code(ConsularDistrict *cons)
{
	if (!cons)
		return 0;
	
	return cons->mae_code;
}

int
consular_district_set_territory_name(ConsularDistrict *cons, const char *value)
{
	if (!cons || !value)
		return -1;
	
	cons->territory_name = strdup(value);

	return 0;
}

const char *
consular_district_get_territory_name(ConsularDistrict *cons)
{
	if (!cons)
		return 0;
	
	return cons->territory_name;
}

int
consular_district_set_sce_code(ConsularDistrict *cons, const char *value)
{
	if (!cons || !value)
		return -1;
	
	cons->sce_code = atol(value);

	return 0;
}

int
consular_district_set_sce_code_int(ConsularDistrict *cons, int value)
{
	if (!cons)
		return -1;
	
	cons->sce_code = value;

	return 0;
}

int
consular_district_get_sce_code(ConsularDistrict *cons)
{
	if (!cons)
		return 0;
	
	return cons->sce_code;
}

void
consular_district_destroy(ConsularDistrict *cons)
{
	if (!cons)
		return;
	
	if (cons->territory_name)
		free(cons->territory_name);

	free(cons);
}
