#ifndef UTILS_H
#define UTILS_H

#ifndef min
# define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
# define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef bool
# define bool short
# define true 1
# define false 0
#endif


/* Provides information about memory usage, measured in pages.  */

typedef struct {
	/* total program size */
	unsigned long size;
	/* resident set size */
	unsigned long resident;
	/* shared pages */
	unsigned long shared;
	/* text (code) */
	unsigned long text;
	/* library (unused in Linux 2.6) */
	unsigned long lib;
	/* data + stack */
	unsigned long data;
	/* dirty pages (unused in Linux 2.6) */
	unsigned long dt;
} memory_status;


char *flatten_argv(int argc, char **argvh);
char *remove_chars(const char *str, const char *rem);
char *remove_spaces(const char *str);

typedef int (*DirEntryHandler) (const char *entry);

int call_over_direntries(const char *dirname, DirEntryHandler handler);

char *get_first_token(char *str, const char *delim);

int get_memory_status(memory_status *status);

char *boolean_to_string(bool value);
bool string_to_boolean(char *value);

#endif  /* UTILS_H */
