#include <assert.h>
#include <libconfig.h>

#include "report.h"
#include "utils.h"  /* bool */
#include "xconfig.h"



/* STATIC configuration object */
static config_t Config;
static bool ConfigOK = false;


void
xconfig_report(const char *caller, const char *path, const char *key)
{
	const char *error_text;
	int error_line;

	if (!ConfigOK)
		return;

	error_line = config_error_line(&Config);
	error_text = config_error_text(&Config);
	
	if (error_line <= 0 || !error_text) {
		assert(path && key);
		report(RPT_ERR, "%s: object not found in config file: %s.%s ",
			caller, path, key);
		return;
	}

	report(RPT_ERR, "%s: error in config file at line %d: %s",
		caller, error_line, error_text);
}


int
xconfig_read_file(const char *filename)
{
	if (ConfigOK) {
		/* the caller must invoke xconfig_clear() first */
		return -1;
	}

	config_init(&Config);
	
	if (config_read_file(&Config, filename) == CONFIG_FALSE) {
		xconfig_report(__FUNCTION__, NULL, NULL);
		return -1;
	}

	ConfigOK = true;

	return 0;
}


void
xconfig_clear(void)
{
	if (!ConfigOK)
		return;

	config_destroy(&Config);
	
	ConfigOK = false;
}


const char *
xconfig_get_string(const char *path, const char *key, int skip, const char *default_value)
{
	config_setting_t *setting;
	const char *value;

	/* NOTE:  SKIP is currently ignored */

	if (!ConfigOK)
		return default_value;
	
	setting = config_lookup(&Config, path);
	if (!setting)
		return default_value;

	if (config_setting_lookup_string(setting, key, &value) == CONFIG_FALSE) {
		xconfig_report(__FUNCTION__, path, key);
		return default_value;
	}
	
	return value;
}


bool
xconfig_get_bool(const char *path, const char *key, int skip, bool default_value)
{
	config_setting_t *setting;
	int value;

	/* NOTE:  SKIP is currently ignored */

	if (!ConfigOK)
		return default_value;

	setting = config_lookup(&Config, path);
	if (!setting)
		return default_value;

	if (config_setting_lookup_bool(setting, key, &value) == CONFIG_FALSE) {
		xconfig_report(__FUNCTION__, path, key);
		return default_value;
	}
	
	return (bool) value;
}


long int
xconfig_get_int(const char *path, const char *key, int skip, long int default_value)
{
	config_setting_t *setting;
	long int value;

	/* NOTE:  SKIP is currently ignored */

	if (!ConfigOK)
		return default_value;

	setting = config_lookup(&Config, path);
	if (!setting)
		return default_value;

	if (config_setting_lookup_int(setting, key, &value) == CONFIG_FALSE) {
		xconfig_report(__FUNCTION__, path, key);
		return default_value;
	}
	
	return value;
}


long long
xconfig_get_int64(const char *path, const char *key, int skip, long long default_value)
{
	config_setting_t *setting;
	long long value;

	/* NOTE:  SKIP is currently ignored */

	if (!ConfigOK)
		return default_value;

	setting = config_lookup(&Config, path);
	if (!setting)
		return default_value;

	if (config_setting_lookup_int64(setting, key, &value) == CONFIG_FALSE) {
		xconfig_report(__FUNCTION__, path, key);
		return default_value;
	}
	
	return value;
}


double
xconfig_get_float(const char *path, const char *key, int skip, double default_value)
{
	config_setting_t *setting;
	double value;

	/* NOTE:  SKIP is currently ignored */

	if (!ConfigOK)
		return default_value;

	setting = config_lookup(&Config, path);
	if (!setting)
		return default_value;

	if (config_setting_lookup_float(setting, key, &value) == CONFIG_FALSE) {
		xconfig_report(__FUNCTION__, path, key);
		return default_value;
	}
	
	return value;
}
