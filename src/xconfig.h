#ifndef XCONFIG_H
#define XCONFIG_H

#include "utils.h"  /* bool */


int xconfig_read_file(const char *filename);
void xconfig_clear(void);

void xconfig_report(const char *caller, const char *path, const char *key);

const char *xconfig_get_string(const char *path, const char *key, int skip, const char *default_value);
bool xconfig_get_bool(const char *path, const char *key, int skip, bool default_value);
long int xconfig_get_int(const char *path, const char *key, int skip, long int default_value);
long long xconfig_get_int64(const char *path, const char *key, int skip, long long default_value);
double xconfig_get_float(const char *path, const char *key, int skip, double default_value);

#endif  /* XCONFIG_H */
