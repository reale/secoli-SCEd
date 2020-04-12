#include <stdio.h>
#include <stdarg.h>

#include <log4c.h>
#include <log4c/appender_type_stream2.h>  /* log4c_stream2_set_fp() */
#include <log4c/layout_type_basic.h>  /* log4c_stream2_set_fp() */

#include "report.h"


static log4c_appender_t *appender = NULL;
static log4c_layout_t *layout = NULL;
static log4c_category_t *category = NULL;
static FILE *log_file = NULL;
    

int
init_reporting(int destination, const char *filename)
{
	if (log4c_init())
		return -1;

	layout = log4c_layout_get("sce_layout");
	if (!layout) {
		destroy_reporting();
		return -1;
	}

	log4c_layout_set_type(layout, &log4c_layout_type_basic);

	if (destination == RPT_DEST_SYSLOG) {
		appender = log4c_appender_get("syslog");
	} else if (destination == RPT_DEST_STDERR) {
		appender = log4c_appender_get("stderr");
	} else if (destination == RPT_DEST_FILE) {
		if (!filename)
			log_file = fopen(REPORT_FILE_DEFAULT, "w");
		else
			log_file = fopen(filename, "w");

		if (!log_file) {
			destroy_reporting();
			return -1;
		}

		appender = log4c_appender_get("sce_log_file");
		log4c_appender_set_type(appender, log4c_appender_type_get("stream2"));
		log4c_stream2_set_fp(appender, log_file);
	} else {
		destroy_reporting();
		return -1;
	}

	log4c_appender_set_layout(appender, layout);
	
	category = log4c_category_get(REPORT_LOG4C_CATEGORY);
	if (!category) {
		destroy_reporting();
		return -1;
	}
	
	log4c_category_set_appender(category, appender);

	log4c_category_set_priority(category, LOG4C_PRIORITY_INFO);

	return 0;
}

void
destroy_reporting(void)
{
	if (category) {
		log4c_category_delete(category);
		category = NULL;
	}

	if (appender) {
		log4c_appender_close(appender);
		appender = NULL;
	}

	if (layout) {
		log4c_layout_delete(layout);
		layout = NULL;
	}

	if (log_file) {
		fclose(log_file);
		log_file = NULL;
	}

	log4c_fini();
}

/*
 * wrapper around log4c_category_vlog()... and a reimplementation
 * of log4c_category_log()
 */

void
report(int priority, const char *format, ... /* args */ )
{
	if (log4c_category_is_priority_enabled(category, priority)) {
		va_list va;
		va_start(va, format);
		log4c_category_vlog(category, priority, format, va);
		va_end(va);
	}
}
