#include <string.h>

#include "location.h"
#include "main.h"  /* COD_STATE_UNSET */
#include "search.h"
#include "sce.h"
#include "xalloc.h"


static int refresh_internal_state(Search *search);


static int
refresh_internal_state(Search *search)
{
	if (!search)
		return -1;

	if (search->name && strlen(search->name) > 0)
		search->search_by = SEARCH_BY_NAME;
	else if (search->zipcode && strlen(search->zipcode) > 0)
		search->search_by = SEARCH_BY_ZIPCODE;
	else if (search->mae_code && search->mae_code != MAE_CODE_UNSET)
		search->search_by = SEARCH_BY_MAE_CODE;
	else if (search->sce_code && search->sce_code != SCE_CODE_UNSET)
		search->search_by = SEARCH_BY_SCE_CODE;
	else
		search->search_by = SEARCH_BY_UNDEFINED;
	
	return 0;
}

Search *
search_create(int cod_state, char *name, long sce_code, char *zipcode, int mae_code)
{
	Search *search;

	search = (Search *) xmalloc(sizeof(Search), __FUNCTION__);
	if (!search)
		return NULL;

	search->locations = queue_create();
	if (!search->locations) {
		search_destroy(search);
		return NULL;
	}

	search->consulates = queue_create();
	if (!search->consulates) {
		search_destroy(search);
		return NULL;
	}

	search->zip_code_specimina = queue_create();
	if (!search->zip_code_specimina) {
		search_destroy(search);
		return NULL;
	}

	search->consular_districts = queue_create();
	if (!search->consular_districts) {
		search_destroy(search);
		return NULL;
	}

	search->cod_state = cod_state;
	search->province = NULL;
	search->territorial_competence = false;

	search->name = name;
	search->sce_code = sce_code;
	search->zipcode = zipcode;
	search->mae_code = mae_code;

	if (refresh_internal_state(search) < 0) {
		search_destroy(search);
		return NULL;
	}

	/* by default search locations */
	search->type = SEARCH_TYPE_LOCATIONS;

	search->constraint = SEARCH_CONSTRAINT_LEFT_EXACT;

	return search;
}

int
search_get_cod_state(Search *search)
{
	if (!search)
		return COD_STATE_UNSET;
	
	return search->cod_state;
}

int
search_set_cod_state(Search *search, int cod_state)
{
	if (!search)
		return -1;
	
	search->cod_state = cod_state;

	return refresh_internal_state(search);
}

int
search_get_mae_code(Search *search)
{
	if (!search)
		return NULL;
	
	return search->mae_code;
}

int
search_set_mae_code(Search *search, char *mae_code)
{
	if (!search)
		return -1;
	
	search->mae_code = atoi(mae_code);

	return refresh_internal_state(search);
}

char *
search_get_name(Search *search)
{
	if (!search)
		return NULL;
	
	return search->name;
}

int
search_set_name(Search *search, char *name)
{
	if (!search)
		return -1;
	
	search->name = name;

	return refresh_internal_state(search);
}

char *
search_get_province(Search *search)
{
	if (!search)
		return NULL;
	
	return search->province;
}

int
search_set_province(Search *search, const char *province)
{
	if (!search)
		return -1;
	
	search->province = strdup(province);

	return refresh_internal_state(search);
}

long
search_get_sce_code(Search *search)
{
	if (!search)
		return NULL;
	
	return search->sce_code;
}

int
search_set_sce_code(Search *search, const char *sce_code)
{
	if (!search)
		return -1;
	
	search->sce_code = strdup(sce_code);

	return refresh_internal_state(search);
}

bool
search_get_territorial_competence(Search *search)
{
	if (!search)
		return false;
	
	return search->territorial_competence;
}

int
search_set_territorial_competence(Search *search, bool value)
{
	if (!search)
		return -1;
	
	search->territorial_competence = value;

	return refresh_internal_state(search);
}

char *
search_get_zipcode(Search *search)
{
	if (!search)
		return NULL;
	
	return search->zipcode;
}

int
search_set_zipcode(Search *search, char *zipcode)
{
	if (!search)
		return -1;
	
	search->zipcode = zipcode;

	return refresh_internal_state(search);
}

int
search_set_search_by(Search *search, int search_by)
{
	if (!search)
		return -1;
	
	search->search_by = search_by;

	return 0;
}

int
search_get_search_by(Search *search)
{
	if (!search)
		return SEARCH_BY_UNDEFINED;
	
	return search->search_by;
}

int
search_set_type(Search *search, int type)
{
	if (!search)
		return -1;
	
	search->type = type;

	return 0;
}

int
search_get_type(Search *search)
{
	if (!search)
		return SEARCH_TYPE_UNDEFINED;
	
	return search->type;
}

int
search_execute(Search *search)
{
	if (!search)
		return -1;
	
	if (search->type != SEARCH_TYPE_ZC_SPECIMINA
	    && search->type != SEARCH_TYPE_CONSULAR_DISTRICTS
	    && search->cod_state == COD_STATE_UNSET)
		return -1;
	
	if (search->type == SEARCH_TYPE_DUMP) {
		search->sce_dump = sce_list_locations(search->cod_state, &(search->sce_dump_count));
		return 0;
	}
	
	/*
	if (search->search_by == SEARCH_BY_NAME)
		if (sce_set_approx_threshold(search->cod_state, search->approx_threshold) < 0)
			return -1;
	*/
	if (search->search_by == SEARCH_BY_NAME && search->type == SEARCH_TYPE_LOCATIONS)
		return sce_find_locations_by_name(search->cod_state, search->name, search->province, search->locations);
	else if (search->search_by == SEARCH_BY_ZIPCODE && search->type == SEARCH_TYPE_LOCATIONS)
		return sce_find_locations_by_zipcode(search->cod_state, search->zipcode, search->locations);
	else if (search->search_by == SEARCH_BY_SCE_CODE && search->type == SEARCH_TYPE_LOCATIONS)
		return sce_find_locations_by_sce_code(search->cod_state, search->sce_code, search->locations);
	else if (search->search_by == SEARCH_BY_MAE_CODE && search->type == SEARCH_TYPE_CONSULAR_DISTRICTS)
		return sce_find_consular_districts_by_mae_code(search->mae_code, search->consular_districts);
	else if (search->type == SEARCH_TYPE_CONSULATES)
		return sce_find_consulates(search->cod_state, search->territorial_competence, search->consulates);
	else if (search->type == SEARCH_TYPE_ZC_SPECIMINA)
		return sce_find_zip_code_specimina(search->zip_code_specimina);
	else
		return -1;
}

Location *
search_pop_location(Search *search)
{
	if (!search)
		return NULL;
	
	return (Location *) queue_pop(search->locations);
}

int
search_count_locations(Search *search)
{
	if (!search)
		return -1;
	
	return queue_get_length(search->locations);
}

Consulate *
search_pop_consulate(Search *search)
{
	if (!search)
		return NULL;
	
	return (Consulate *) queue_pop(search->consulates);
}

int
search_count_consulates(Search *search)
{
	if (!search)
		return -1;
	
	return queue_get_length(search->consulates);
}

ConsularDistrict *
search_pop_consular_district(Search *search)
{
	if (!search)
		return NULL;
	
	return (ConsularDistrict *) queue_pop(search->consular_districts);
}

int
search_count_consular_districts(Search *search)
{
	if (!search)
		return -1;
	
	return queue_get_length(search->consular_districts);
}

Zipcode *
search_pop_zip_code_specimen(Search *search)
{
	if (!search)
		return NULL;
	
	return (Zipcode *) queue_pop(search->zip_code_specimina);
}

int
search_count_zip_code_specimina(Search *search)
{
	if (!search)
		return -1;
	
	return queue_get_length(search->zip_code_specimina);
}

int
search_count_results(Search *search)
{
	if (!search)
		return -1;

	if (search->type == SEARCH_TYPE_LOCATIONS)
		return search_count_locations(search);
	else if (search->type == SEARCH_TYPE_CONSULAR_DISTRICTS)
		return search_count_consular_districts(search);
	else if (search->type == SEARCH_TYPE_CONSULATES)
		return search_count_consulates(search);
	else if (search->type == SEARCH_TYPE_ZC_SPECIMINA)
		return search_count_zip_code_specimina(search);
	else
		return -1;
}

void
search_destroy(Search *search)
{
	if (!search)
		return;

	if (search->locations)
		queue_destroy(search->locations);

	if (search->consulates)
		queue_destroy(search->consulates);

	if (search->zip_code_specimina)
		queue_destroy(search->zip_code_specimina);

	if (search->province)
		free(search->province);

	free(search);
}
