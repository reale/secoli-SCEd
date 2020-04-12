#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "daemon.h"
#include "report.h"


static void child_is_ok(int signal);



pid_t
daemon_fork(void)
{
	pid_t child;
	pid_t parent;
	int child_status;
	struct sigaction sa;

	debug(RPT_DEBUG, "%s()", __FUNCTION__);

	parent = getpid();
	debug(RPT_INFO, "parent = %d", parent);

	/* Install signal handler for the child's signal.
	   We'll reset it in the child.  */
	sa.sa_handler = child_is_ok;
	sigemptyset(&(sa.sa_mask));
	sa.sa_flags = SA_RESTART;
	sigaction(SIGUSR1, &sa, NULL);

	/*** FORK ***/
	switch ((child = fork())) {

	  case -1: /* error!  */
		report(RPT_ERR, "Could not fork");
		return -1;

	  case 0: /* the child */
		break;

	  default: /* the parent */
		debug(RPT_INFO, "child = %d", child);
		wait(&child_status);

		/* BUG? According to the man page wait() should also return
		 * when a signal comes in that is caught. Instead it
		 * continues to wait. */

		if (WIFEXITED(child_status)) {
			/* Child exited normally, let's exit with the same status.  */
			debug(RPT_INFO, "Child has terminated!");
			exit(WEXITSTATUS(child_status));
		}

		/* Child has signalled everything is OK, let's exit.  */
		debug(RPT_INFO, "Got OK signal from child.");
		exit(EXIT_SUCCESS);
	}

	/* From now on we are the CHILD.  */

	/* Reset signal handler */
	sa.sa_handler = SIG_DFL;
	sigaction(SIGUSR1, &sa, NULL);

	/*
	 * IMPORTANT Create a new session to avoid catching a SIGHUP when the
	 * shell is closed.
	 */
	setsid();

	return parent;
}

int
daemon_say_ok_to_parent(pid_t parent_pid)
{
	/* Tell the parent everything is OK.  */

	debug(RPT_DEBUG, "%s(parent_pid=%d)", __FUNCTION__, parent_pid);

	kill(parent_pid, SIGUSR1);

	return 0;
}

static void
child_is_ok(int signal)
{
	/* Catch this signal to be sure the child is OK.  */

	debug(RPT_INFO, "%s(signal=%d)", __FUNCTION__, signal);

	/* Exit immediately.  */
	_exit(EXIT_SUCCESS);
}
