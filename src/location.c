#include <stdio.h>
#include <string.h>

#include "location.h"
#include "xalloc.h"

Location *
location_create(void)
{
	Location *loc = xmalloc(sizeof(Location), __FUNCTION__);

	loc->sce_code = 0;
	loc->cons_mae_code = 0;
	loc->zip_code = NULL;
	memset(loc->province, 0, 3);
	loc->istat_code = 0;
	loc->synonym = NULL;
	loc->name = NULL;
	loc->full_name = NULL;

	return loc;
}

int
location_set_sce_code(Location *loc, const char *value)
{
	if (!loc || !value)
		return -1;
	
	loc->sce_code = atol(value);

	return 0;
}

int
location_set_cons_mae_code(Location *loc, const char *value)
{
	if (!loc || !value)
		return -1;
	
	loc->cons_mae_code = atoi(value);

	return 0;
}

int
location_set_zip_code(Location *loc, const char *value)
{
	if (!loc || !value)
		return -1;
	
	loc->zip_code = strdup(value);

	return 0;
}

int
location_set_province(Location *loc, const char *value)
{
	if (!loc || !value)
		return -1;
	
	strncpy(loc->province, value, 2);
	loc->province[2] = '\0';

	return 0;
}

int
location_set_istat_code(Location *loc, const char *value)
{
	if (!loc || !value)
		return -1;
	
	loc->istat_code = atoi(value);

	return 0;
}

int
location_set_synonym(Location *loc, const char *value)
{
	if (!loc || !value)
		return -1;
	
	loc->synonym = strdup(value);

	return 0;
}

int
location_set_name(Location *loc, const char *value)
{
	if (!loc || !value)
		return -1;
	
	loc->name = strdup(value);

	return 0;
}

int
location_set_full_name(Location *loc, const char *value)
{
	if (!loc || !value)
		return -1;
	
	loc->full_name = strdup(value);

	return 0;
}

int
location_refresh_full_name(Location *loc)
{
	size_t name_l, synonym_l, full_name_l;

	if (!loc)
		return -1;
	
	if (loc->full_name)
		free(loc->full_name);
	
	if (!loc->synonym)
		synonym_l = 0;
	else
		synonym_l = strlen(loc->synonym);

	if (!synonym_l) {
		loc->full_name = NULL;
	} else {
		if (!loc->name)
			name_l = 0;
		else
			name_l = strlen(loc->name);

		if (!name_l) {
			loc->full_name = strdup(loc->synonym);
		} else {
			full_name_l = synonym_l + name_l + 4;

			loc->full_name = xmalloc(full_name_l, __FUNCTION__);
			if (!loc->full_name)
				return -1;

			snprintf(loc->full_name, full_name_l, "%s (%s)", loc->synonym, loc->name);
		}
	}

	return 0;
}

long
location_get_sce_code(Location *loc)
{
	if (!loc)
		return 0;
	
	return loc->sce_code;
}

int
location_get_cons_mae_code(Location *loc)
{
	if (!loc)
		return 0;
	
	return loc->cons_mae_code;
}

char *
location_get_zip_code(Location *loc)
{
	if (!loc)
		return NULL;
	
	return loc->zip_code;
}

char *
location_get_province(Location *loc)
{
	if (!loc)
		return strdup("EE");
	
	return loc->province;
}

int
location_get_istat_code(Location *loc)
{
	if (!loc)
		return 0;
	
	return loc->istat_code;
}

char *
location_get_synonym(Location *loc)
{
	if (!loc)
		return NULL;
	
	return loc->synonym;
}

char *
location_get_name(Location *loc)
{
	if (!loc)
		return NULL;
	
	return loc->name;
}

char *
location_get_full_name(Location *loc)
{
	if (!loc)
		return NULL;
	
	return loc->full_name;
}

void
location_destroy(Location *loc)
{
	if (!loc)
		return;

	if (loc->zip_code)
		free(loc->zip_code);
	if (loc->synonym)
		free(loc->synonym);
	if (loc->name)
		free(loc->name);
	if (loc->full_name)
		free(loc->full_name);
	
	free(loc);
}
