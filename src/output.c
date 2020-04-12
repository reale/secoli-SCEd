#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "output.h"
#include "utils.h"  /* bool */
#include "xalloc.h"

#include "output_csv.h"
#include "output_flat.h"
#include "output_json.h"
#include "output_void.h"


static OutputHandler handlers[] = {
	{ "CSV",  output_csv_open,  output_csv_print,  output_csv_close  },
	{ "FLAT", output_flat_open, output_flat_print, output_flat_close },
	{ "JSON", output_json_open, output_json_print, output_json_close },
	{ "VOID", output_void_open, output_void_print, output_void_close },
	{ NULL, NULL, NULL, NULL }
};


OutputHandler *
output_create_handler(Client *c, const char *cmd)
{
	OutputHandler *OH;
	int i, found = 0;

	if (!cmd)
		return NULL;

	for (i = 0; handlers[i].description != NULL; i++) {
		if (strcmp(cmd, handlers[i].description) == 0) {
			found = 1;
			break;
		}
	}

	if (!found)
		return NULL;

	OH = xmalloc(sizeof(OutputHandler), __FUNCTION__);
	if (!OH)
		return NULL;
	
	OH->description = strdup(handlers[i].description);
	OH->opener = handlers[i].opener;
	OH->dumper = handlers[i].dumper;
	OH->closer = handlers[i].closer;

	OH->status = OUTPUT_HANDLER_STATUS_UNSET;

	if (output_open_handler(OH, NULL) < 0) {
		output_destroy_handler(OH);
		return NULL;
	}

	OH->c = c;

	OH->priv_data = NULL;

	return OH;
}

void
output_destroy_handler(OutputHandler *OH)
{
	if (!OH)
		return;
	
	output_close_handler(OH);

	OH->status = OUTPUT_HANDLER_STATUS_UNSET;
	
	if (OH->description)
		free(OH->description);
	
	free(OH);

	return;
}

char *
output_get_handler_description(OutputHandler *OH)
{
	if (!OH)
		return NULL;
	else
		return OH->description;
}

int
output_open_handler(OutputHandler *OH, void *params)
{
	int retval;

	if (!OH)
		return -1;
	
	if (OH->status == OUTPUT_HANDLER_STATUS_OPEN)
		return 0;

	retval = OH->opener(params);
	if (retval == 0)
		OH->status = OUTPUT_HANDLER_STATUS_OPEN;
	
	return retval;
}

int
output_run_handler(OutputHandler *OH, Search *search, bool print_result_count)
{
	if (!OH || !search)
		return -1;
	
	return OH->dumper(OH->c, search, print_result_count);
}

int
output_close_handler(OutputHandler *OH)
{
	int retval;

	if (!OH)
		return -1;
	
	if (OH->status == OUTPUT_HANDLER_STATUS_CLOSED)
		return 0;

	retval = OH->closer();
	if (retval == 0)
		OH->status = OUTPUT_HANDLER_STATUS_CLOSED;
	
	return retval;
}
