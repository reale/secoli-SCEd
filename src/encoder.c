#include <errno.h>
#include <iconv.h>
#include <string.h>

#include "encoder.h"
#include "iconv.h"
#include "report.h"
#include "xalloc.h"


// http://www.tbray.org/ongoing/When/200x/2003/04/06/Unicode


Encoder *
encoder_create(const char *charset, int direction)
{
	char *input_charset, *output_charset;
	Encoder *enc;
	iconv_t handler;

	if (direction == ENCODER_DIRECTION_INPUT) {
		input_charset = strdup(charset);
		output_charset = strdup(ENCODER_DEFAULT_CHARSET);
	} else if (direction == ENCODER_DIRECTION_OUTPUT) {
		input_charset = strdup(ENCODER_DEFAULT_CHARSET);
		output_charset = strdup(charset);
	} else {
		return NULL;
	}
		
	if (!output_charset)
		return NULL;
	
	enc = xmalloc(sizeof(Encoder), __FUNCTION__);
	if (!enc)
		return NULL;

	handler = iconv_open(output_charset, input_charset);

	if ((int) handler < 0) {
		if (errno == EINVAL)
			report(RPT_ERR, "%s: Conversion from '%s' to '%s' is not supported",
				__FUNCTION__, input_charset, output_charset);
		else
			report(RPT_ERR, "%s: Initialization failure: %s",
				__FUNCTION__, strerror(errno));

		free(enc);
		return NULL;
	}
	
	enc->handler = handler;
	enc->input_charset = input_charset;
	enc->output_charset = output_charset;

	return enc;
}

void
encoder_destroy(Encoder *enc)
{
	if (!enc)
		return;
	
	if (enc->input_charset)
		free(enc->input_charset);
	if (enc->output_charset)
		free(enc->output_charset);
	
	if (iconv_close(enc->handler))
		report(RPT_ERR, "%s: iconv_close failed: %s", strerror(errno));
	
	free(enc);
}

char *
encoder_run(Encoder *enc, char *input)
{
	size_t input_len, output_len;
	size_t iconv_count;

	char *output;

	size_t input_left_len, output_left_len;
    	char *input_left, *output_left;


    	if (!input)
		return NULL;
	
	input_len = strlen(input);
	if (!input_len)
		return NULL;

	/* Assign enough room for the output string.  */
	output_len = 4 * input_len;
	output = xcalloc(output_len, 1, __FUNCTION__);

	input_left = input;
	input_left_len = input_len;
	output_left = output;
	output_left_len = output_len;

	iconv_count = iconv(enc->handler, &input_left, &input_left_len, &output_left, &output_left_len);

    #if 0
    /* Handle failures. */
    if (iconv_value == (size_t) -1) {
	fprintf (stderr, "iconv failed: in string '%s', length %d, "
		"out string '%s', length %d\n",
		 euc, len, utf8start, utf8len);
	switch (errno) {
	    /* See "man 3 iconv" for an explanation. */
	case EILSEQ:
	    fprintf (stderr, "Invalid multibyte sequence.\n");
	    break;
	case EINVAL:
	    fprintf (stderr, "Incomplete multibyte sequence.\n");
	    break;
	case E2BIG:
	    fprintf (stderr, "No more room.\n");
	    break;
	default:
	    fprintf (stderr, "Error: %s.\n", strerror (errno));
	}
	exit (1);
    }
    #endif

	return output;
}

char *
encoder_get_output_charset(Encoder *enc)
{
	if (!enc)
		return NULL;
	
	return enc->output_charset;
}

Encoder *
encoder_set_output_charset(Encoder *enc, const char *charset)
{
	Encoder *new_enc;

	if (!enc)
		return NULL;
	
	new_enc = encoder_create(charset, ENCODER_DIRECTION_OUTPUT);
	if (!new_enc)
		return NULL;

	/* only at this point we destroy the original encoder */
	encoder_destroy(enc);

	return new_enc;
}
