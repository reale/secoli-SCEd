#include <stdlib.h>
#include <jansson.h>

#include "client.h"
#include "location.h"
#include "output_json.h"
#include "report.h"
#include "search.h"
#include "sock.h"
#include "utils.h"  /* bool */


/*
 * Documentation for the Jansson library at:  http://www.digip.org/jansson/doc/2.4/
 */

static int dump_sce(Client *c, Search *search, json_t **jroot);
static int print_locations(Client *c, Search *search, json_t **jroot);


static inline void json_report(json_error_t error, const char *caller)
{
	report(RPT_WARNING, "%s: JSON error at position %z: %s", caller, error.position, error.text);
}

static int
dump_sce(Client *c, Search *search, json_t **jroot)
{
	int found = 0;
	json_error_t jerror;
	json_t *jdata, *jname;
	int i;

	/* create JSON container */
	jdata = json_array();
	if (!jdata) {
		json_report(jerror, __FUNCTION__);
		return -1;
	}

	for (i = 0; i < search->sce_dump_count; i++) {

		jname = json_string(search->sce_dump[i]);
		if (!jname) {
			json_report(jerror, __FUNCTION__);
			continue;
		}

		if (json_array_append(jdata, jname) < 0) {
			json_report(jerror, __FUNCTION__);
			continue;
		}

		found = 1;
	}

	*jroot = jdata;

	return found;
}

static int
print_locations(Client *c, Search *search, json_t **jroot)
{
	Location *loc;
	int found = 0;
	json_error_t jerror;
	json_t *jdata;


	/* create JSON container */
	jdata = json_object();
	if (!jdata) {
		json_report(jerror, __FUNCTION__);
		return -1;
	}

			/*
			location_get_sce_code(loc), delimiter,
			location_get_cons_mae_code(loc), delimiter,
			location_get_province(loc), delimiter,
			location_get_istat_code(loc), delimiter,
			encoded_name);
			*/

	while (1) {
		json_t *jlocations, *jname;

		char *name, *encoded_name;
		char *mae_code;

		loc = search_pop_location(search);

		if (!loc)
			break;
 
		name = location_get_name(loc);
		mae_code = xmalloc(8, __FUNCTION__);

		if (!name || !mae_code) {
			location_destroy(loc);

			/* TODO: report error */
			continue;
		}

		//snprintf(mae_code, 8, "%d", location_get_cons_mae_code(loc));

		jlocations = json_object_get(jdata, mae_code);
		if (!jlocations) {
			jlocations = json_array();

			if (!jlocations) {
				location_destroy(loc);
				json_report(jerror, __FUNCTION__);
				continue;
			}

			if (json_object_set_new(jdata, mae_code, jlocations) < 0) {
				location_destroy(loc);
				json_report(jerror, __FUNCTION__);
				continue;
			}
		}

		encoded_name = client_run_output_enc(c, name);
		if (!encoded_name) {
			location_destroy(loc);
			json_decref(jlocations);
			json_report(jerror, __FUNCTION__);
			continue;
		}

		jname = json_string(encoded_name);
		if (!jname) {
			location_destroy(loc);
			json_decref(jlocations);
			json_report(jerror, __FUNCTION__);
			continue;
		}

		if (json_array_append_new(jlocations, jname) < 0) {
			location_destroy(loc);
			json_decref(jlocations);
			json_report(jerror, __FUNCTION__);
			continue;
		}

		location_destroy(loc);

		found = 1;
	}

	*jroot = jdata;

	return found;
}

int
output_json_open(void *params)
{
	return 0;
}

int
output_json_print(Client *c, Search *search, bool print_result_count)
{
	int search_type;
	int found = 0;
	char *output_buffer;
	json_t *jroot;

	jroot = malloc(sizeof(json_t));

	/* print_result_count is ignored, of course */

	if (!c || !search)
		return -1;

	search_type = search_get_type(search);


	if (search_type == SEARCH_TYPE_DUMP)
		found = dump_sce(c, search, &jroot);
	else if (search_type == SEARCH_TYPE_LOCATIONS)
		found = print_locations(c, search, &jroot);

	if (jroot) {
		output_buffer = json_dumps(jroot, 0);
		sock_printf_huge(client_get_socket(c), "%s\n", output_buffer);
		free(output_buffer);

		/* destroy JSON container */
		json_decref(jroot);
	}

	return found;
}

int
output_json_close(void)
{
	return 0;
}
