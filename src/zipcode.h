#ifndef ZIPCODE_H
#define ZIPCODE_H

typedef struct Zipcode {
	int cod_state;
	char *code;
} Zipcode;

Zipcode *zipcode_create(void);
void zipcode_destroy(Zipcode *zipcode);

int zipcode_set_cod_state(Zipcode *zipcode, const char *value);
int zipcode_set_cod_state_integer(Zipcode *zipcode, int value);
int zipcode_get_cod_state(Zipcode *zipcode);
int zipcode_set_code(Zipcode *zipcode, const char *value);
const char *zipcode_get_code(Zipcode *zipcode);

#endif  /* ZIPCODE_H */
