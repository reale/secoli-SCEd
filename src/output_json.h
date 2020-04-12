#ifndef OUTPUT_JSON_H
#define OUTPUT_JSON_H

#include "client.h"
#include "search.h"
#include "utils.h"  /* bool */

int output_json_open(void *params);
int output_json_print(Client *c, Search *search, bool print_result_count);
int output_json_close(void);

#endif  /* OUTPUT_JSON_H */
