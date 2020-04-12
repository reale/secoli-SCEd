#ifndef OUTPUT_VOID_H
#define OUTPUT_VOID_H

#include "client.h"
#include "search.h"
#include "utils.h"  /* bool */

int output_void_open(void *params);
int output_void_print(Client *c, Search *search, bool print_result_count);
int output_void_close(void);

#endif  /* OUTPUT_VOID_H */
