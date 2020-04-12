#ifndef OUTPUT_FLAT_H
#define OUTPUT_FLAT_H

#include "client.h"
#include "search.h"
#include "utils.h"  /* bool */

int output_flat_open(void *params);
int output_flat_print(Client *c, Search *search, bool print_result_count);
int output_flat_close(void);

#endif  /* OUTPUT_FLAT_H */
