#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/wait.h>

#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/types.h>
#include <limits.h>



#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

#include "report.h"

#include "daemon.h"
#include "sce.h"
#include "sock.h"
#include "client.h"
#include "parse.h"
#include "identity.h"
#include "xconfig.h"
#include "sce.h"
#include "main.h"
#include "help.h"
#include "xsqlite.h"
#include "utils.h"  /* bool */




/* Store some standard defines into vars... */
char *version = VERSION;
char *build_date = __DATE__;



unsigned int bind_port = UNSET_INT;
char bind_addr[64];	/* Do not preinit these strings as they will occupy */
char configfile[256];	/* a lot of space in the executable. */
char scepath[256];	/* a lot of space in the executable. */  // XXX
char user[64];		/* The values will be overwritten anyway... */


/* End of configuration variables */

/* Local variables */
static int foreground_mode = UNSET_INT;

static int stored_argc;
static char **stored_argv;
static volatile bool got_reload_signal = 0;

/* Local exported variables */
long timer = 0;

/**** Local functions ****/
static void clear_settings(void);
static void do_main_loop(void);
static int process_command_line(int argc, char **argv);
static int process_configfile(char *cfgfile);
static int set_default_settings(void);
static int install_signal_handlers(int catch_reload);
static int init_sce(void);
static int drop_privs(char *user);
static void do_reload(void);
//static void exit_program(int signum);
static void catch_reload_signal(int val);
static void output_banner(void);

#define CHAIN(e,f) { if (e>=0) { e=(f); }}
#define CHAIN_END(e,msg) { if (e<0) { report(RPT_CRIT,(msg)); exit(EXIT_FAILURE); }}


int
main(int argc, char **argv)
{
	int e = 0;
	pid_t parent_pid = 0;

	stored_argc = argc;
	stored_argv = argv;

	/*
	 * Settings in order of preference:
	 *
	 * 1: Command line
	 * 2: Configuration file
	 * 3: Default
	 *
	 */

	/* set reporting-related settings */

	/* say hello...  */
	/*
	report(RPT_NOTICE, "SCE gateway version %s starting", version);
	report(RPT_INFO, "Built on %s, protocol version %s", build_date, PROTOCOL_VERSION);
	*/

	clear_settings();

	/* process command line*/
	CHAIN(e, process_command_line(argc, argv));

	/* process the configuration file;  if an option is read therein
	 * and not already set, then set it */
	if (strcmp(configfile, UNSET_STR) == 0)
		strncpy(configfile, DEFAULT_CONFIGFILE, sizeof(configfile));
	CHAIN(e, process_configfile(configfile));

	/* if a parameter is not set, then set it to the default value */
	CHAIN(e, set_default_settings());

	if (!foreground_mode)
		init_reporting(RPT_DEST_SYSLOG, NULL);
	else
		init_reporting(RPT_DEST_STDERR, NULL);

	//set_reporting("SCEd", report_level, report_dest);

	identity_init();  /* TODO:  add to CHAIN processing */

	CHAIN_END(e, "Critical error while processing settings, abort.");

	/* go into daemon mode (if requested) */
	if (!foreground_mode) {
		report(RPT_INFO, "Server forking to background");
		CHAIN(e, parent_pid = daemon_fork());
	} else {
		output_banner();
		report(RPT_INFO, "Server running in foreground");
	}

	/* install signal handlers */
	CHAIN(e, install_signal_handlers(!foreground_mode));

	/* init database connections */
	CHAIN(e, init_sce());

	/* init socket */
	CHAIN(e, sock_init(bind_addr, bind_port));

	/* init clients */
	CHAIN(e, clients_init_all());

	CHAIN_END(e, "Critical error while initializing, abort.");

	if (!foreground_mode) {
		/* startup went OK */
		daemon_say_ok_to_parent(parent_pid);
	}

	/* switch to an unprivileged user */
	drop_privs(user);

	topology_init(); // XXX

	/* LOOP FOREVER */
	do_main_loop();

	return 0;
}


static void
clear_settings(void)
{
	debug(RPT_DEBUG, "%s()", __FUNCTION__);

	bind_port = UNSET_INT;
	strncpy(bind_addr, UNSET_STR, sizeof(bind_addr));
	strncpy(configfile, UNSET_STR, sizeof(configfile));
	strncpy(user, UNSET_STR, sizeof(user));
	foreground_mode = UNSET_INT;
}

static int
process_command_line(int argc, char **argv)
{
	int c, b;
	int e = 0, help = 0;

	debug(RPT_DEBUG, "%s(argc=%d, argv=...)", __FUNCTION__, argc);

	/* Reset getopt */
	opterr = 0;

	while ((c = getopt(argc, argv, "hc:d:fa:p:u:s:r:i:")) > 0) {
		switch(c) {
			case 'h':
				help = 1;
				break;
			case 'c':
				strncpy(configfile, optarg, sizeof(configfile));
				configfile[sizeof(configfile)-1] = '\0';
				break;
	 		case 'd':
				/* XXX */
				if (1 < 10) {
					{
						e = -1;
					}
				} else {
					e = -1;
				}
				break;
			case 'f':
				foreground_mode = 1;
				break;
			case 'a':
				strncpy(bind_addr, optarg, sizeof(bind_addr));
				bind_addr[sizeof(bind_addr)-1] = '\0';
				break;
			case 'p':
				bind_port = atoi(optarg);
				break;
			case 'u':
				strncpy(user, optarg, sizeof(user));
				user[sizeof(user)-1] = '\0';
				break;
			case '?':
				/* getopt() returns '?' when an option argument is missing */
				report(RPT_ERR, "Unknown option: '%c'", optopt);
				e = -1;
				break;
			case ':':
				report(RPT_ERR, "Missing option argument!");
				e = -1;
				break;
		}
	}

	if (optind < argc) {
		report(RPT_ERR, "Non-option arguments on the command line !");
		e = -1;
	}
	if (help) {
		help_screen();
		e = -1;
	}
	return e;
}


/* reads and parses configuration file */
static int
process_configfile(char *configfile)
{
	debug(RPT_DEBUG, "%s()", __FUNCTION__);

	/* Read server settings*/

	if (xconfig_read_file(configfile) != 0) {
		report(RPT_CRIT, "Could not read config file: %s", configfile);
		return -1;
	}

	if (strcmp(bind_addr, UNSET_STR) == 0)
		strncpy(bind_addr, xconfig_get_string("Server", "BindAddress", 0, UNSET_STR), sizeof(bind_addr));

	if (bind_port == UNSET_INT)
		bind_port = xconfig_get_int("Server", "BindPort", 0, UNSET_INT);

	if (strcmp(user, UNSET_STR) == 0)
		strncpy(user, xconfig_get_string("Server", "User", 0, UNSET_STR), sizeof(user));

	if (foreground_mode == UNSET_INT) {
		int fg = xconfig_get_bool("Server", "Foreground", 0, UNSET_INT);

		if (fg != UNSET_INT)
			foreground_mode = fg;
	}

	return 0;
}


#define PROCESS_FREQ 32

static void
do_main_loop(void)
{
	struct timeval t;
	struct timeval last_t;
	int sleeptime;
	long int process_lag = 0;
	long int t_diff;

	debug(RPT_DEBUG, "%s()", __FUNCTION__);

	/* initial time */
	gettimeofday(&t, NULL);

	while (1) {
		/* current time */
		last_t = t;
		gettimeofday(&t, NULL);

		t_diff = t.tv_sec - last_t.tv_sec;

		if ( ((t_diff + 1) > (LONG_MAX / 1e6)) || (t_diff < 0) ) {
			/* overflow */
			t_diff = 0;
			process_lag = 1;
		} else {
			t_diff *= 1e6;
			t_diff += t.tv_usec - last_t.tv_usec;
		}

                process_lag += t_diff;

		if (process_lag > 0) {
			
			/* check for expired timeouts */
			clients_check_timeouts(t_diff);

			/* poll clients */
			sock_poll_clients();

			/* parse messages (if there are any...) */
			parse_all_client_messages();

			process_lag = 0 - (1e6/PROCESS_FREQ);
		}


		/* sleep */
		sleeptime = 0 - process_lag;
		if (sleeptime > 0) {
			usleep(sleeptime);
		}

		/* pay attention to HANGUP signals */
		if (got_reload_signal) {
			got_reload_signal = 0;
			do_reload();
		}
	}


	exit_program(0);
}

static int
set_default_settings(void)
{
	debug(RPT_DEBUG, "%s()", __FUNCTION__);

	if (bind_port == UNSET_INT)
		bind_port = DEFAULT_BIND_PORT;
	if (strcmp(bind_addr, UNSET_STR) == 0)
		strncpy(bind_addr, DEFAULT_BIND_ADDR, sizeof(bind_addr));
	if (strcmp(user, UNSET_STR) == 0)
		strncpy(user, DEFAULT_USER, sizeof(user));

	if (foreground_mode == UNSET_INT)
		foreground_mode = DEFAULT_FOREGROUND_MODE;

	return 0;
}


static int
install_signal_handlers(int catch_reload)
{
	/* install signal handlers */

	struct sigaction sa;

	debug(RPT_DEBUG, "%s(catch_reload=%d)", __FUNCTION__, catch_reload);

	sigemptyset(&(sa.sa_mask));

	/* ignore SIGPIPE */
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL);

	/* orderly exit on SIGINT and SIGTERM */
	sa.sa_handler = exit_program;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	/* catch SIGHUP only if requested */
	if (catch_reload) {
		sa.sa_handler = catch_reload_signal;
	}
	else {
		/* treat SIGHUP just like SIGINT/SIGTERM */
	}
	sigaction(SIGHUP, &sa, NULL);

	return 0;
}

static int
init_sce(void)
{
	debug(RPT_DEBUG, "%s()", __FUNCTION__);

	sce_init();  // XXX check return value

	if (sce_load() < 0) {
		report(RPT_ERR, "Could not load SCE");
		return -1;
	}

	return 0;
}

static int
drop_privs(char *user)
{
	struct passwd *pwent;

	debug(RPT_DEBUG, "%s(user=\"%.40s\")", __FUNCTION__, user);

	if (getuid() == 0 || geteuid() == 0) {
		if ((pwent = getpwnam(user)) == NULL) {
			report(RPT_ERR, "User %.40s not a valid user!", user);
			return -1;
		} else {
			if (setuid(pwent->pw_uid) < 0) {
				report(RPT_ERR, "Unable to switch to user %.40s", user);
				return -1;
			}
		}
	}

	return 0;
}


static void
do_reload(void)
{
	int e = 0;

	sce_unload();
	xconfig_clear();
	clear_settings();

	/* Reread command line*/
	CHAIN(e, process_command_line(stored_argc, stored_argv));

	/* Reread config file */
	if (strcmp(configfile, UNSET_STR)==0)
		strncpy(configfile, DEFAULT_CONFIGFILE, sizeof(configfile));
	CHAIN(e, process_configfile(configfile));

	/* Set default values */
	CHAIN(e, (set_default_settings(), 0));

	CHAIN(e, init_sce());
	CHAIN_END(e, "Critical error while reloading, abort.");
}

void
exit_program(int signum)
{
	char buf[64];

	debug(RPT_DEBUG, "%s(signum=%d)", __FUNCTION__, signum);

	if (signum < 0) {
		report(RPT_NOTICE, "Authenticated user requested shutdown");

	} else if (signum > 0) {
		strncpy(buf, "Server shutting down on ", sizeof(buf));

		switch(signum) {
			case 1:
				strcat(buf, "SIGHUP");
				break;
			case 2:
				strcat(buf, "SIGINT");
				break;
			case 15:
				strcat(buf, "SIGTERM");
				break;
			default:
				snprintf(buf, sizeof(buf), "Server shutting down on signal %d", signum);
				break;
		}

		report(RPT_NOTICE, buf);
	}

	/* shutdown clients */
	clients_shutdown_all();

	/* shutdown the sockets */
        sock_shutdown();

	/* close database connections */
	sce_unload();

	/* say bye bye...  */
	report(RPT_INFO, "Exiting...");

	_exit(EXIT_SUCCESS);
}

static void
catch_reload_signal(int val)
{
	debug(RPT_DEBUG, "%s(val=%d)", __FUNCTION__, val);

	got_reload_signal = 1;
}

static void
output_banner(void)
{
	fprintf(stderr, "SCE Gateway %s, Protocol %s\n", VERSION, PROTOCOL_VERSION);
}
