#ifndef SCE_H
#define SCE_H

#include <sqlite3.h>

#include "location.h"
#include "queue.h"
#include "utils.h"  /* bool */

#define COD_STATE_UNSET -1
#define COD_STATE_UK 75
#define COD_STATE_ITALY 97

#define SCE_ENTITIES_MAX 1024
#define SCE_LOCATIONS_MAX 1000000


typedef struct SceEntity {

	/* sqlite3 connection */
	sqlite3 *db;

	/* filtertree */
	struct FilterTree *FT;

	/* SCE dump */
	Location **locations;
	int location_count;

} SceEntity;


typedef int (*SceEntityHandler) (SceEntity *entity, void *data);

int sce_init(void);

int sce_load(void);
void sce_unload(void);

int sce_call_over_entities(SceEntityHandler handler, void *data);

int sce_validate_cod_state(const char *string);
int sce_got_italian_cod_state(int cod_state);

Location **sce_list_locations(int cod_state, int *count);

int sce_find_locations_by_name(int cod_state, const char *name, const char *province, Queue *locations);
int sce_find_locations_by_sce_code(int cod_state, long sce_code, Queue *locations);
int sce_find_locations_by_zipcode(int cod_state, const char *zipcode, Queue *locations);
int sce_find_consulates(int cod_state, bool territorial_competence, Queue *consulates);
int sce_find_zip_code_specimina(Queue *zip_code_specimina);
int sce_find_consular_districts_by_mae_code(int mae_code, Queue *consular_districts);


#endif  /* SCE_H */
