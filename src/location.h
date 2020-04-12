#ifndef LOCATION_H
#define LOCATION_H

typedef struct Location {

	//int   state_territory_sce_code;

	long  sce_code;
	int   cons_mae_code;
	char *zip_code;
	char  province[3];
	int   istat_code;
	char *synonym;
	char *name;

	char *full_name;


	// char *reduced_name;
	// int syn_id;
} Location;

Location * location_create(void);
void location_destroy(Location *loc);

int location_set_sce_code(Location *loc, const char *value);
int location_set_cons_mae_code(Location *loc, const char *value);
int location_set_zip_code(Location *loc, const char *value);
int location_set_province(Location *loc, const char *value);
int location_set_istat_code(Location *loc, const char *value);
int location_set_synonym(Location *loc, const char *value);
int location_set_name(Location *loc, const char *value);
int location_set_full_name(Location *loc, const char *value);
int location_refresh_full_name(Location *loc);

long location_get_sce_code(Location *loc);
int location_get_cons_mae_code(Location *loc);
char *location_get_zip_code(Location *loc);
char *location_get_province(Location *loc);
int location_get_istat_code(Location *loc);
char *location_get_synonym(Location *loc);
char *location_get_name(Location *loc);
char *location_get_full_name(Location *loc);

#endif  /* LOCATION_H */
