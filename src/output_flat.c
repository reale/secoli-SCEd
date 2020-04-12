#include "client.h"
#include "output_csv.h"
#include "output_flat.h"
#include "search.h"
#include "sock.h"
#include "utils.h"  /* bool */


int
output_flat_open(void *params)
{
	return 0;
}

int
output_flat_print(Client *c, Search *search, bool print_result_count)
{
	int search_type, search_by;
	int found = 0;

	if (!c || !search)
		return -1;

	search_type = search_get_type(search);
	search_by = search_get_search_by(search);

	if (search_type == SEARCH_TYPE_DUMP)
		found = output_csv_dump_sce(c, search, print_result_count, " ");
	else if (search_type == SEARCH_TYPE_LOCATIONS && search_by == SEARCH_BY_SCE_CODE)
		found = output_csv_print_locations_with_zip_codes(c, search, print_result_count, " ");
	else if (search_type == SEARCH_TYPE_LOCATIONS)
		found = output_csv_print_locations_with_names(c, search, print_result_count, " ");
	else if (search_type == SEARCH_TYPE_CONSULAR_DISTRICTS)
		found = output_csv_print_consular_districts(c, search, print_result_count, " ");
	else if (search_type == SEARCH_TYPE_CONSULATES)
		found = output_csv_print_consulates(c, search, print_result_count, " ");
	else if (search_type == SEARCH_TYPE_ZC_SPECIMINA)
		found = output_csv_print_zip_code_specimina(c, search, print_result_count, " ");

	return found;
}

int
output_flat_close(void)
{
	return 0;
}
