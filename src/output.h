#ifndef OUTPUT_H
#define OUTPUT_H

#include "client.h"
#include "search.h"
#include "utils.h"  /* bool */


#define OUTPUT_DEFAULT_HANDLER "FLAT"

#define OUTPUT_HANDLER_STATUS_UNSET  0
#define OUTPUT_HANDLER_STATUS_OPEN   1
#define OUTPUT_HANDLER_STATUS_CLOSED 2


typedef struct Client Client;  /* Client and OutputHandler reference each other */

typedef int (*OutputOpener) (void *params);
typedef int (*OutputDumper) (Client *c, Search *search, bool print_result_count);
typedef int (*OutputCloser) (void);

typedef struct OutputHandler {

	char *description;

	OutputOpener opener;
	OutputDumper dumper;
	OutputCloser closer;

	int status;

	Client *c;

	void *priv_data;

} OutputHandler;


OutputHandler *output_create_handler(Client *c, const char *cmd);
void output_destroy_handler(OutputHandler *OH);

char *output_get_handler_description(OutputHandler *OH);
int output_open_handler(OutputHandler *OH, void *params);
int output_close_handler(OutputHandler *OH);
int output_run_handler(OutputHandler *OH, Search *search, bool print_result_count);

#endif  /* OUTPUT_H */
