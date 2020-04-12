#ifndef CONSULAR_DISTRICT_H
#define CONSULAR_DISTRICT_H

typedef struct ConsularDistrict {
	int mae_code;
	const char *territory_name;
	int sce_code;
} ConsularDistrict;

ConsularDistrict *consular_district_create(void);
void consular_district_destroy(ConsularDistrict *cons);

int consular_district_set_mae_code(ConsularDistrict *cons, const char *value);
int consular_district_set_mae_code_int(ConsularDistrict *cons, int value);
int consular_district_get_mae_code(ConsularDistrict *cons);
int consular_district_set_territory_name(ConsularDistrict *cons, const char *value);
const char *consular_district_get_territory_name(ConsularDistrict *cons);
int consular_district_set_sce_code(ConsularDistrict *cons, const char *value);
int consular_district_set_sce_code_int(ConsularDistrict *cons, int value);
int consular_district_get_sce_code(ConsularDistrict *cons);


#endif  /* CONSULAR_DISTRICT_H */
