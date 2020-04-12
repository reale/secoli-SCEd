#ifndef REPORT_H
#define REPORT_H

#include <log4c.h>


/* Reporting levels */

enum priority_level_t {
 	/* Critical condition: the program stops */
	RPT_CRIT    = LOG4C_PRIORITY_FATAL,
	/* Error condition: serious problem */
	RPT_ERR     = LOG4C_PRIORITY_ERROR,
	/* Warning condition */
	RPT_WARNING = LOG4C_PRIORITY_WARN,
	/* Major event in the program */
	RPT_NOTICE  = LOG4C_PRIORITY_NOTICE,
	/* Minor event in the program */
	RPT_INFO    = LOG4C_PRIORITY_INFO,
	/* Debug */
	RPT_DEBUG   = LOG4C_PRIORITY_DEBUG
};

/* Reporting destinations */

#define RPT_DEST_STDERR  0
#define RPT_DEST_SYSLOG  1
#define RPT_DEST_FILE    2

/* Defaults */
#define REPORT_LOG4C_CATEGORY "SCEd"
#define REPORT_FILE_DEFAULT "/var/log/sce.log"


int init_reporting(int destination, const char *filename);
void destroy_reporting(void);

void report(int priority, const char *format, ... /*args*/);
static inline void dont_report(int priority, const char *format, ... /*args*/) {}

#ifdef DEBUG
#  define debug report
#else
#  define debug dont_report
#endif

#endif  /* REPORT_H */
