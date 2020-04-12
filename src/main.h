#ifndef MAIN_H
#define MAIN_H

#ifndef VERSION
#  define VERSION "0.3.3"
#endif

#define PROTOCOL_VERSION "0"

#ifndef SCED_PORT
#  define SCED_PORT 5005
#endif

extern char *version;
extern char *protocol_version;
extern char *build_date;

extern long timer;

/**** Configuration variables ****/

extern unsigned int bind_port;
extern char bind_addr[];
extern char configfile[];
extern char user[];


/* End of configuration variables */

/* Defines for having 'unset' values*/
#define UNSET_INT	-1
#define UNSET_STR	"\01"


#if !defined(SYSCONFDIR)
# define SYSCONFDIR "/etc"
#endif


/*
 * default values
 */

#define DEFAULT_BIND_ADDR		"127.0.0.1"
#define DEFAULT_BIND_PORT		SCED_PORT
#define DEFAULT_CONFIGFILE		SYSCONFDIR "/SCEd.cfg"
#define DEFAULT_USER			"nobody"
#define DEFAULT_SCE_PATH		""
#define DEFAULT_FOREGROUND_MODE		0

void exit_program(int signum);

#endif  /* MAIN_H */
