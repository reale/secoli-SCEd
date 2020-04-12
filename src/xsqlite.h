#ifndef SQLITE_H
#define SQLITE_H

#include <sqlite3.h>

#include "location.h"
#include "queue.h"
#include "utils.h"  /* bool */


int xsqlite_load_datafile(sqlite3 **db, const char *filename);
void xsqlite_close_connection(sqlite3 *db);
int xsqlite_get_memory_used(void);

int xsqlite_exec_ddl(sqlite3 *db, const char *ddl);

int xsqlite_list_locations(sqlite3 *db, Location **locations, int locations_max, bool with_codmae);

int xsqlite_find_locations_by_name(sqlite3 *db, const char *name, const char *province, bool in_italy, Queue *locations);
int xsqlite_find_locations_by_sce_code(sqlite3 *db, long sce_code, bool in_italy, Queue *locations);
int xsqlite_find_locations_by_zipcode(sqlite3 *db, const char *zipcode, int cod_state, Queue *locations);
int xsqlite_find_consulates(sqlite3 *db, int cod_state, bool territorial_competence, Queue *consulates);
int xsqlite_find_zip_code_specimina(sqlite3 *db, Queue *zip_code_specimina);
int xsqlite_find_zip_code_specimen_italy(Queue *zip_code_specimina);
int xsqlite_find_consular_districts(sqlite3 *db, Queue *consular_districts);

#endif  /* XSQLITE_H */
