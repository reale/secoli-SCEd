#ifndef OUTPUT_CSV_H
#define OUTPUT_CSV_H

#include "client.h"
#include "search.h"
#include "utils.h"  /* bool */

#define OUTPUT_CSV_DEFAULT_DELIMITER ";"

int output_csv_open(void *params);
int output_csv_print(Client *c, Search *search, bool print_result_count);
int output_csv_close(void);

int output_csv_dump_sce(Client *c, Search *search, bool print_result_count, const char *delimiter);
int output_csv_print_locations_with_names(Client *c, Search *search, bool print_result_count, const char *delimiter);
int output_csv_print_locations_with_zip_codes(Client *c, Search *search, bool print_result_count, const char *delimiter);
int output_csv_print_consular_districts(Client *c, Search *search, bool print_result_count, const char *delimiter);
int output_csv_print_consulates(Client *c, Search *search, bool print_result_count, const char *delimiter);

#endif  /* OUTPUT_CSV_H */
