#include <stdlib.h>

#include "client.h"
#include "location.h"
#include "output_csv.h"
#include "search.h"
#include "sock.h"
#include "utils.h"  /* bool */


int
output_csv_dump_sce(Client *c, Search *search, bool print_result_count, const char *delimiter)
{
	char *cod_mae, *name;
	char *encoded_name;
	int found = 0;
	int i;


	if (print_result_count)
		sock_printf(client_get_socket(c), "%d\n", search->sce_dump_count); // TODO: search_count_sce_dump()

	for (i = 0; i < search->sce_dump_count; i++) {

		Location *loc = search->sce_dump[i];

		cod_mae = location_get_cons_mae_code(loc);
		name = location_get_full_name(loc);

		encoded_name = client_run_output_enc(c, name);
		if (!encoded_name) {
			/* TODO: report error */
			continue;
		}

		sock_printf(client_get_socket(c), "%s%s%s\n", cod_mae, delimiter, encoded_name);

		free(encoded_name);

		/* IMPORTANT:  do not destroy the location as it is referenced to by others  */

		found = 1;
	}

	return found;
}

int
output_csv_print_locations_with_names(Client *c, Search *search, bool print_result_count, const char *delimiter)
{
	Location *loc;
	int found = 0;


	if (print_result_count)
		sock_printf(client_get_socket(c), "%d\n", search_count_locations(search));

	while (1) {

		char *name;
		char *encoded_name;

		loc = search_pop_location(search);

		if (!loc)
			break;

		name = location_get_full_name(loc);

		if (!name) {
			location_destroy(loc);

			/* TODO: report error */
			continue;
		}

		encoded_name = client_run_output_enc(c, name);
		if (!encoded_name) {
			location_destroy(loc);

			/* TODO: report error */
			continue;
		}

		sock_printf(client_get_socket(c), 
			"%012d%s%07d%s%s%s%06d%s%s\n",
			location_get_sce_code(loc), delimiter,
			location_get_cons_mae_code(loc), delimiter,
			location_get_province(loc), delimiter,
			location_get_istat_code(loc), delimiter,
			encoded_name);

		free(encoded_name);

		location_destroy(loc);

		found = 1;
	}

	return found;
}

int
output_csv_print_locations_with_zip_codes(Client *c, Search *search, bool print_result_count, const char *delimiter)
{
	Location *loc;
	int found = 0;


	if (print_result_count)
		sock_printf(client_get_socket(c), "%d\n", search_count_locations(search));

	while (1) {

		char *zip_code;

		loc = search_pop_location(search);

		if (!loc)
			break;

		zip_code = location_get_zip_code(loc);

		if (!zip_code) {
			location_destroy(loc);

			/* TODO: report error */
			continue;
		}

		sock_printf(client_get_socket(c), 
			"%012d%s%07d%s%s%s%06d%s%s\n",
			location_get_sce_code(loc), delimiter,
			location_get_cons_mae_code(loc), delimiter,
			location_get_province(loc), delimiter,
			location_get_istat_code(loc), delimiter,
			zip_code);

		location_destroy(loc);

		found = 1;
	}

	return found;
}

int
output_csv_print_consular_districts(Client *c, Search *search, bool print_result_count, const char *delimiter)
{
	Consulate *cons;
	int found = 0;


	if (print_result_count)
		sock_printf(client_get_socket(c), "%d\n", search_count_consular_districts(search));

	while (1) {

		char *name;
		char *encoded_name;

		cons = search_pop_consular_district(search);

		if (!cons)
			break;

		name = consular_district_get_territory_name(cons);

		if (!name) {
			consular_district_destroy(cons);

			/* TODO: report error */
			continue;
		}

		encoded_name = client_run_output_enc(c, name);
		if (!encoded_name) {
			consular_district_destroy(cons);

			/* TODO: report error */
			continue;
		}

		sock_printf(client_get_socket(c), 
			"%03d%s%s\n",
			consular_district_get_sce_code(cons), delimiter,
			encoded_name);

		free(encoded_name);

		consular_district_destroy(cons);

		found = 1;
	}

	return found;
}

int
output_csv_print_consulates(Client *c, Search *search, bool print_result_count, const char *delimiter)
{
	Consulate *cons;
	int found = 0;


	if (print_result_count)
		sock_printf(client_get_socket(c), "%d\n", search_count_consulates(search));

	while (1) {

		cons = search_pop_consulate(search);

		if (!cons)
			break;

		sock_printf(client_get_socket(c), "%07d\n", consulate_get_mae_code(cons));

		consulate_destroy(cons);

		found = 1;
	}

	return found;
}

int
output_csv_print_zip_code_specimina(Client *c, Search *search, bool print_result_count, const char *delimiter)
{
	Zipcode *zipcode;
	int found = 0;


	if (print_result_count)
		sock_printf(client_get_socket(c), "%d\n", search_count_zip_code_specimina(search));

	while (1) {

		zipcode = search_pop_zip_code_specimen(search);

		if (!zipcode)
			break;

		sock_printf(client_get_socket(c), "%03d %s\n", zipcode_get_cod_state(zipcode), zipcode_get_code(zipcode));

		zipcode_destroy(zipcode);

		found = 1;
	}

	return found;
}

int
output_csv_open(void *params)
{
	return 0;
}

int
output_csv_print(Client *c, Search *search, bool print_result_count)
{
	int search_type, search_by;
	int found = 0;

	if (!c || !search)
		return -1;

	search_type = search_get_type(search);
	search_by = search_get_search_by(search);

	if (search_type == SEARCH_TYPE_DUMP)
		found = output_csv_dump_sce(c, search, print_result_count, client_get_field_delimiter(c));
	else if (search_type == SEARCH_TYPE_LOCATIONS && search_by == SEARCH_BY_SCE_CODE)
		found = output_csv_print_locations_with_zip_codes(c, search, print_result_count, client_get_field_delimiter(c));
	else if (search_type == SEARCH_TYPE_LOCATIONS)
		found = output_csv_print_locations_with_names(c, search, print_result_count, client_get_field_delimiter(c));
	else if (search_type == SEARCH_TYPE_CONSULAR_DISTRICTS)
		found = output_csv_print_consular_districts(c, search, print_result_count, client_get_field_delimiter(c));
	else if (search_type == SEARCH_TYPE_CONSULATES)
		found = output_csv_print_consulates(c, search, print_result_count, client_get_field_delimiter(c));
	else if (search_type == SEARCH_TYPE_ZC_SPECIMINA)
		found = output_csv_print_zip_code_specimina(c, search, print_result_count, client_get_field_delimiter(c));

	return found;
}

int
output_csv_close(void)
{
	return 0;
}
