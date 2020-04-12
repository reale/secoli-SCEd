#ifndef SEARCH_H
#define SEARCH_H

#include "consular_district.h"
#include "consulate.h"
#include "location.h"
#include "queue.h"
#include "utils.h"  /* bool */
#include "zipcode.h"


#define SEARCH_BY_UNDEFINED  0
#define SEARCH_BY_NAME       1
#define SEARCH_BY_SCE_CODE   2
#define SEARCH_BY_ZIPCODE    3
#define SEARCH_BY_MAE_CODE    4

#define SEARCH_TYPE_UNDEFINED           0
#define SEARCH_TYPE_CONSULATES          1
#define SEARCH_TYPE_LOCATIONS           2
#define SEARCH_TYPE_MUNICIPALITY        3
#define SEARCH_TYPE_ZC_SPECIMINA        4
#define SEARCH_TYPE_CONSULAR_DISTRICTS  5
#define SEARCH_TYPE_DUMP                9

#define SEARCH_CONSTRAINT_UNDEFINED    0
#define SEARCH_CONSTRAINT_LEFT_EXACT   1
#define SEARCH_CONSTRAINT_RIGHT_EXACT  2
#define SEARCH_CONSTRAINT_EXACT        3
#define SEARCH_CONSTRAINT_NONE         99

#define SCE_CODE_UNSET  -1
#define MAE_CODE_UNSET  -1


typedef struct Search {

	/* predicate: name */
	char *name;

	/* predicate: zip code */
	char *zipcode;

	/* predicate: province */
	char *province;

	/* predicate: cod_state */
	int cod_state;

	/* predicate: sce_code */
	long sce_code;

	/* predicate: mae_code */
	int mae_code;

	int search_by;
	int type;
	const char **sce_dump;
	int sce_dump_count;
	int constraint;
	bool territorial_competence;

	bool in_italy;

	Queue *locations;
	Queue *consulates;
	Queue *zip_code_specimina;
	Queue *consular_districts;
} Search;

Search *search_create(int cod_state, char *name, long sce_code, char *zipcode, int mae_code);
int search_execute(Search *search);
void search_destroy(Search *search);

int search_get_cod_state(Search *search);
int search_set_cod_state(Search *search, int cod_state);

int search_get_mae_code(Search *search);
int search_set_mae_code(Search *search, char *mae_code);

char *search_get_name(Search *search);
int search_set_name(Search *search, char *name);

char *search_get_province(Search *search);
int search_set_province(Search *search, const char *province);

long search_get_sce_code(Search *search);
int search_set_sce_code(Search *search, const char *sce_code);

int search_set_search_by(Search *search, int search_by);
int search_get_search_by(Search *search);

bool search_get_territorial_competence(Search *search);
int search_set_territorial_competence(Search *search, bool value);

int search_get_type(Search *search);
int search_set_type(Search *search, int type);

char *search_get_zipcode(Search *search);
int search_set_zipcode(Search *search, char *zipcode);

int search_count_locations(Search *search);
Location *search_pop_location(Search *search);

int search_count_consulates(Search *search);
Consulate *search_pop_consulate(Search *search);

int search_count_consular_districts(Search *search);
ConsularDistrict *search_pop_consular_district(Search *search);

int search_count_zip_code_specimina(Search *search);
Zipcode *search_pop_zip_code_specimen(Search *search);

int search_count_results(Search *search);

#endif  /* SEARCH_H */
