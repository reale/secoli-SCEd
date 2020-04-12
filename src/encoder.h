#ifndef ENCODER_H
#define ENCODER_H

#include <iconv.h>

#define ENCODER_DIRECTION_UNSET 0
#define ENCODER_DIRECTION_INPUT 1
#define ENCODER_DIRECTION_OUTPUT 2

#define ENCODER_DEFAULT_CHARSET "UTF-8"

typedef struct Encoder {
	char *input_charset;
	char *output_charset;
	iconv_t handler;
} Encoder;

Encoder *encoder_create(const char *charset, int direction);
void encoder_destroy(Encoder *enc);
char *encoder_run(Encoder *enc, char *input);

char *encoder_get_output_charset(Encoder *enc);
Encoder *encoder_set_output_charset(Encoder *enc, const char *charset);

#endif /* ENCODER_H */
