#ifndef CONSULATE_H
#define CONSULATE_H

typedef struct Consulate {
	int mae_code;
} Consulate;

Consulate *consulate_create(void);
void consulate_destroy(Consulate *cons);

int consulate_set_mae_code(Consulate *cons, const char *value);
int consulate_get_mae_code(Consulate *cons);

#endif  /* CONSULATE_H */
