#include <stdio.h>

#include "help.h"
#include "main.h"


void
help_screen(void)
{
	fprintf(stdout, "SCE Gateway %s, Protocol %s\n", VERSION, PROTOCOL_VERSION);
	fprintf(stdout, "Usage: SCEd [<options>]\n");
	fprintf(stdout, "  where <options> are:\n");
	fprintf(stdout, "    -h                  Display this help screen\n");
	fprintf(stdout, "    -c <config>         Use the specified configuration path [default: %s]\n",
		DEFAULT_CONFIGFILE);
	fprintf(stdout, "    -d <dir>            Load SCE datafiles from the path specified [default: %s]\n",
		DEFAULT_SCE_PATH);
	fprintf(stdout, "    -f                  Run in the foreground\n");
	fprintf(stdout, "    -a <addr>           Network (IP) address to bind to [default: %s]\n",
		DEFAULT_BIND_ADDR);
	fprintf(stdout, "    -p <port>           Network port to listen for connections on [default: %i]\n",
		DEFAULT_BIND_PORT);
	fprintf(stdout, "    -u <user>           User to run as [default: %s]\n",
		DEFAULT_USER);
}
