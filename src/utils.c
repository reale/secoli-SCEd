#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>  /* PATH_MAX */

#include "report.h"
#include "utils.h"
#include "xalloc.h"


char *
flatten_argv(int argc, char **argv)
{
	int i;
	size_t inc_length = 0;
	char *buffer = NULL;
	
	for (i = 0; i < argc; i++) {
		size_t length = strlen(argv[i]);
		buffer = xrealloc(buffer, inc_length+length+1, __FUNCTION__);
		memcpy(buffer+inc_length, argv[i], length);
		buffer[inc_length+length] = ' ';
		inc_length += length+1;
	}

	buffer[inc_length-1] = '\0';

	return buffer;
}


char *
get_first_token(char *str, const char *delim)
{
	char *str_1, *token, *saveptr;
	int j;

	for (j = 1, str_1 = str; j < 2; j++, str_1 = NULL) {
		token = strtok_r(str_1, delim, &saveptr);
		if (token == NULL)
			break;
	}

	return token;
}


/*
 * remove from STR all character listed in REM
 */

char *
remove_chars(const char *str, const char *rem)
{
	char *buffer;
	int i;

	if (!str)
		return NULL;

	buffer = xmalloc(strlen(str) + 1, __FUNCTION__);
	
	for (i = 0; *str != '\0'; str++) {
		const char *p = rem;
		int found = 0;

		for (; *p != '\0'; p++) {
			if (*str == *p) found = 1;
		}

		if (found == 0) {
			buffer[i] = *str;
			i++;
		}
	}

	buffer[i] = '\0';

	if (strlen(buffer) == 0) {
		free(buffer);
		return NULL;
	}

	return buffer;
}


/*
 * remove from STR duplicate tabs and spaces
 */

char *
remove_spaces(const char *str)
{
	const char spaces[] = " \t";
	char *buffer;
	int i;

	if (!str)
		return NULL;

	buffer = xmalloc(strlen(str) + 1, __FUNCTION__);
	
	for (i = 0; *str != '\0'; str++) {
		const char *p = spaces;
		int found = 0;

		for (; *p != '\0'; p++) {
			if (*str == *p) found = 1;
		}

		if (found == 0)
			buffer[i] = *str;
		else
			buffer[i] = ' ';

		i++;
	}

	buffer[i] = '\0';

	if (strlen(buffer) == 0) {
		free(buffer);
		return NULL;
	}

	return buffer;
}


int
call_over_direntries(const char *dirname, DirEntryHandler handler)
{
	DIR *d;
	char filename[PATH_MAX+1];


	d = opendir(dirname);

	if (!d) {
		report(RPT_ERR, "%s: Cannot open directory '%s': %s",
				__FUNCTION__, dirname, strerror (errno));
		return -1;
	}


	while (1) {
		struct dirent *entry;
		const char *d_name;

		entry = readdir(d);
		if (!entry) {
			/* no more entries */
			break;
		}

		d_name = entry->d_name;

		if (entry->d_type & DT_REG) {
			snprintf(filename, PATH_MAX, "%s/%s", dirname, d_name);
			if (handler(filename) < 0) {
				/* TODO:  report */
				continue;
			}
		}
	}

	closedir(d);

	return 0;
}


int
get_memory_status(memory_status *status)
{
	const char *statm_path = "/proc/self/statm";
	FILE *f;
	
	if (!status)
		return -1;
	
	f = fopen(statm_path, "r");
	
	if (!f) {
		/* TODO: report */
		return -1;
	}
	
	if (7 != fscanf(f, "%lu %lu %lu %lu %lu %lu %lu",
		&(status->size),
		&(status->resident),
		&(status->shared),
		&(status->text),
		&(status->lib),
		&(status->data),
		&(status->dt)))
	{
		/* TODO: report */
		return -1;
	}

	return 0;

}


char *
boolean_to_string(bool value)
{
	return (value ? strdup("ENABLED") : strdup("DISABLED"));
}

bool
string_to_boolean(char *value)
{
	if (!strcmp(value, "ENABLED"))
		return true;
	else
		return false;  /* XXX */
}
