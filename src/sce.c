#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <sqlite3.h>
#include <ctype.h>

#include "consular_district.h"
#include "location.h"
#include "queue.h"
#include "report.h"
#include "sce.h"
#include "utils.h"
#include "xalloc.h"
#include "xsqlite.h"
 


static SceEntity *SceEntityTable[SCE_ENTITIES_MAX];
static SceEntity *SceEntity000;
static SceEntity *SceEntityItaly;
static Queue *global_consular_districts = NULL;

static char *filename2cod_state(const char *filename);
static int load_entity(const char *filename);
static int load_consular_districts(SceEntity *entity, void *data);
static int load_entity_skip_validation(const char *filename, int cod_state);
static int unload_entity(SceEntity *entity, void *data);
static inline int set_entity_by_cod_state(int cod_state, SceEntity *entity);
static inline SceEntity *get_entity_by_cod_state(int cod_state);
static inline int sce_find_zip_code_specimen_per_state(SceEntity *entity, void *data);
static int sce_find_zip_code_specimen_italy(Queue *zip_code_specimina);
static inline int sce_find_consular_districts_per_state(SceEntity *entity, void *data);


int
sce_init(void)
{
	int i;

	SceEntity000 = NULL;
	SceEntityItaly = NULL;
	for (i = 0; i < SCE_ENTITIES_MAX; i++) {
		SceEntityTable[i] = NULL;
	}

	return 0;
}

int
sce_load(void)
{
	const char dirname[] = "/usr/local/share/sce";  // XXX
	const char sce_entity_000[] = "/usr/local/share/sce/cmae.000.mth";  // XXX
	const char sce_entity_italy[] = "/usr/local/share/sce/cmae.AAA.mth";  // XXX

	if (call_over_direntries(dirname, load_entity) < 0) {
		sce_unload();
		return -1;
	}

	global_consular_districts = queue_create();  /* XXX */
	if (sce_call_over_entities(sce_find_consular_districts_per_state, global_consular_districts) < 0) {
		sce_unload();
		return -1;
	}

	if (load_entity_skip_validation(sce_entity_000, 0) < 0) {
		sce_unload();
		return -1;
	}

	if (load_entity_skip_validation(sce_entity_italy, COD_STATE_ITALY) < 0) {
		sce_unload();
		return -1;
	}

	return 0;
}

void
sce_unload(void)
{
	if (SceEntity000) {
		unload_entity(SceEntity000, NULL);
	}
	if (SceEntityItaly) {
		unload_entity(SceEntityItaly, NULL);
	}

	sce_call_over_entities(unload_entity, NULL);
}

int
sce_call_over_entities(SceEntityHandler handler, void *data)
{
	SceEntity *entity;
	int i;

	for (i = 0; i < SCE_ENTITIES_MAX; i++) {
		entity = SceEntityTable[i];

		if (entity)
			handler(entity, data);
	}

	return 0;
}

static char *
filename2cod_state(const char *filename)
{
	const char *p;
	char *buf, *bufsave;

	if (!filename)
		return NULL;

	buf = xmalloc(sizeof(filename)+1, __FUNCTION__);
	bufsave = buf;

	for (p = filename; *p != '\0'; p++) {
		if (isdigit(*p))
			*buf++ = *p;
	}

	*buf = '\0';

	return bufsave;
}

static int
load_entity(const char *filename)
{
	int cod_state;


	report(RPT_DEBUG, "%s(filename=\"%.80s\")", __FUNCTION__, filename);

	printf("%s\n", filename); // XXX

	if (!filename)
		return -1;
	
	cod_state = sce_validate_cod_state(filename2cod_state(filename));

	if (cod_state == COD_STATE_UNSET)
		return -1;
	
	return load_entity_skip_validation(filename, cod_state);
}

static int
load_consular_districts(SceEntity *entity, void *data)
{
	if (!entity)
		return -1;


	/* DATA refers to CONSULAR_DISTRICTS */

	return xsqlite_find_zip_code_specimina(entity->db, (Queue *) data);
}

static int
load_entity_skip_validation(const char *filename, int cod_state)
{
	sqlite3 *db;
	SceEntity *entity;
	Location **locations;
	int location_count;


	report(RPT_DEBUG, "%s(filename=\"%.80s\")", __FUNCTION__, filename);

	if (!filename || cod_state == COD_STATE_UNSET)
		return -1;
	
	/* load data file */
	if (xsqlite_load_datafile(&db, filename) < 0)
		return -1;

	/* dump locations */
	/*
	locations = xmalloc(SCE_LOCATIONS_MAX*sizeof(Location *), __FUNCTION__);
	if (!locations) {
		xsqlite_close_connection(db);
		return -1;
	}
	location_count = xsqlite_list_locations(db, locations, SCE_LOCATIONS_MAX, true);
	*/

	entity = xmalloc(sizeof(SceEntity), __FUNCTION__);
	if (!entity) {
		/* XXX: free, etc. */
		return -1;
	}

	entity->db = db;
	//entity->locations = locations;
	//entity->location_count = location_count;

	return set_entity_by_cod_state(cod_state, entity);
}

static int
unload_entity(SceEntity *entity, void *data)
{
	/* always return success */

	if (!entity)
		return 0;
	
	xsqlite_close_connection(entity->db);

	return 0;
}

static inline int
set_entity_by_cod_state(int cod_state, SceEntity *entity)
{
	if (!entity)
		return -1;

	if (cod_state == 0)
		SceEntity000 = entity;
	else if (sce_got_italian_cod_state(cod_state))
		SceEntityItaly = entity;
	else
		SceEntityTable[cod_state] = entity;

	return 0;
}

static inline SceEntity *
get_entity_by_cod_state(int cod_state)
{
	if (cod_state == 0)
		return SceEntity000;
	else if (sce_got_italian_cod_state(cod_state))
		return SceEntityItaly;
	else
		return SceEntityTable[cod_state];  // XXX validate
}

Location **
sce_list_locations(int cod_state, int *count)
{
	SceEntity *e = get_entity_by_cod_state(cod_state);
	if (!e)
		return NULL;
	
	*count = e->location_count;

	return e->locations;
}

int
sce_find_locations_by_name(int cod_state, const char *name, const char *province, Queue *locations)
{
	SceEntity *e = get_entity_by_cod_state(cod_state);
	if (!e)
		return -1;

	/*
	if (e->FT) {
		const char *results[10000];
		printf("%s\n", name);
		int j = filtertree_query(e->FT, name, results, 100);
		printf("%d\n", j);

		int i;
		for (i = 0; i < j; i++)
		{
		printf("%s\n", results[i]);
			xsqlite_find_locations_by_name(e->db, results[i], locations);
		}
	}
	*/

	xsqlite_find_locations_by_name(e->db, name, province, sce_got_italian_cod_state(cod_state), locations);
	return 0;
}

int
sce_find_locations_by_sce_code(int cod_state, long sce_code, Queue *locations)
{
	SceEntity *e = get_entity_by_cod_state(cod_state);
	if (!e)
		return -1;

	return xsqlite_find_locations_by_sce_code(e->db, sce_code, sce_got_italian_cod_state(cod_state), locations);
}

int
sce_find_locations_by_zipcode(int cod_state, const char *zipcode, Queue *locations)
{
	SceEntity *e = get_entity_by_cod_state(cod_state);
	if (!e)
		return -1;

	return xsqlite_find_locations_by_zipcode(e->db, zipcode, cod_state, locations);
}

int
sce_find_consulates(int cod_state, bool territorial_competence, Queue *consulates)
{
	SceEntity *e;

	if (sce_got_italian_cod_state(cod_state))
		return -1;

	if (territorial_competence) {
		e = get_entity_by_cod_state(cod_state);
		if (!e)
			return -1;

		return xsqlite_find_consulates(e->db, cod_state, territorial_competence, consulates);
	} else {
		return xsqlite_find_consulates(SceEntity000->db, cod_state, territorial_competence, consulates);
	}
}

int
sce_find_consular_districts_by_mae_code(int mae_code, Queue *consular_districts)
{
	ConsularDistrict *cons;
	int target_mae_code;

	for (cons = queue_get_first(global_consular_districts); cons; cons = queue_get_next(global_consular_districts)) {
		if (cons) {

			target_mae_code = consular_district_get_mae_code(cons);

			if (target_mae_code == mae_code) {
				ConsularDistrict *new_cons;

				new_cons = consular_district_create();

				consular_district_set_mae_code_int(new_cons, target_mae_code);
				consular_district_set_territory_name(new_cons, consular_district_get_territory_name(cons));
				consular_district_set_sce_code_int(new_cons, consular_district_get_sce_code(cons));

				queue_push(consular_districts, new_cons);
			}
		}
	}

	return 0;
}

static inline int
sce_find_consular_districts_per_state(SceEntity *entity, void *data)
{
	if (!entity)
		return -1;

	/* DATA refers to CONSULAR_DISTRICTS */

	return xsqlite_find_consular_districts(entity->db, (Queue *) data);
}

int
sce_find_zip_code_specimina(Queue *zip_code_specimina)
{
	if (sce_find_zip_code_specimen_italy(zip_code_specimina))
		return -1;

	if (sce_call_over_entities(sce_find_zip_code_specimen_per_state, zip_code_specimina) < 0)
		return -1;

	return 0;
}

static inline int
sce_find_zip_code_specimen_per_state(SceEntity *entity, void *data)
{
	if (!entity)
		return -1;

	/* DATA refers to ZIP_CODE_SPECIMINA */

	return xsqlite_find_zip_code_specimina(entity->db, (Queue *) data);
}

static int
sce_find_zip_code_specimen_italy(Queue *zip_code_specimina)
{
	return xsqlite_find_zip_code_specimen_italy(zip_code_specimina);
}

int
sce_got_italian_cod_state(int cod_state)
{
	return (cod_state == COD_STATE_ITALY);
}

int
sce_validate_cod_state(const char *string)
{
	const char *p = string;
	int cod_state;

	if (!string)
		return COD_STATE_UNSET;

	while (*p != '\0') {
		if (!isdigit(*p++)) {
			return COD_STATE_UNSET;  // XXX test
		}
	}

	cod_state = atoi(string);

	if ((cod_state > 0 && cod_state < SCE_ENTITIES_MAX) || sce_got_italian_cod_state(cod_state))
		return cod_state;
	else
		return COD_STATE_UNSET;
}
