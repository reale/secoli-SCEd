#define _GNU_SOURCE  /* vasprintf() */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include "client.h"
#include "report.h"
#include "sock.h"
#include "sock2.h"


int
sock_init(char* bind_addr, int bind_port)
{
	return sock2_init(bind_addr, bind_port);
}


int
sock_shutdown(void)
{
	return sock2_shutdown();
}


int
sock_create_inet_socket(char *addr, unsigned int port)
{
	return sock2_create_inet_socket(addr, port);
}


int
sock_poll_clients(void)
{
	return sock2_poll_clients();
}


int
sock_destroy_client_socket(Client *c)
{
	return sock2_destroy_client_socket(c);
}


int
sock_close(int fd)
{
	return sock2_close(fd);
}


int
sock_recv(int fd, void *dest, size_t maxlen)
{
	return sock2_recv(fd, dest, maxlen);
}


int
sock_recv_string(int fd, char *dest, size_t maxlen)
{
	return sock2_recv_string(fd, dest, maxlen);
}


int
sock_send(int fd, void *src, size_t size)
{
	return sock2_send(fd, src, size);
}


int
sock_printf(int fd, const char *format, ... /*args*/)
{
	char buf[SOCK_MAX_BUFLEN];
	va_list ap;
	int size = 0;

	va_start(ap, format);
	size = vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	if (size < 0) {
		report(RPT_ERR, "%s: vsnprintf failed", __FUNCTION__);
		return -1;
	}
	if (size > sizeof(buf))
		report(RPT_WARNING, "%s: vsnprintf truncated message", __FUNCTION__);

	return sock_send_string(fd, buf);
}


int
sock_printf_huge(int fd, const char *format, ... /*args*/)
{
	char *buf;
	va_list ap;
	int size = 0;

	va_start(ap, format);
	size = vasprintf(&buf, format, ap);
	va_end(ap);

	if (size < 0) {
		report(RPT_ERR, "%s: vasprintf failed", __FUNCTION__);
		return -1;
	}

	size = sock_send_string(fd, buf);

	free(buf);

	return size;
}


int
sock_answer_ok(int fd)
{
	if (fd != SOCK_UNSET)
		return sock_printf(fd, "%s\n", SOCK_ANSWER_OK);
	else
		return -1;
}


int
sock_answer_undefined(int fd)
{
	if (fd != SOCK_UNSET)
		return sock_printf(fd, "%s\n", SOCK_ANSWER_UNDEFINED);
	else
		return -1;
}


int
sock_answer_unavailable(int fd)
{
	if (fd != SOCK_UNSET)
		return sock_printf(fd, "%s\n", SOCK_ANSWER_UNAVAILABLE);
	else
		return -1;
}


int
sock_answer_unimplemented(int fd)
{
	if (fd != SOCK_UNSET)
		return sock_printf(fd, "%s\n", SOCK_ANSWER_UNIMPLEMENTED);
	else
		return -1;
}

int
sock_send_string(int fd, char *string)
{
	return sock_send(fd, string, strlen(string));
}


char *
sock_geterror(void)
{
	return strerror(errno);
}
