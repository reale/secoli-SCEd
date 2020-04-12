/* socket internals */

#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>

#include "client.h"
#include "queue.h"
#include "report.h"
#include "sock.h"
#include "sock2.h"
#include "ring.h"
#include "utils.h"  /* max() & min() */
#include "xalloc.h"





#define MAXMSG SOCK_MAX_BUFLEN

static fd_set active_fd_set, read_fd_set;
static int listening_fd;

static Queue *openSocketQueue = NULL;
static Queue *freeClientSocketQueue = NULL;

static Ring *messageRing;

typedef struct _ClientSocketMap
{
	int socket;
	Client *client;
} ClientSocketMap;


ClientSocketMap *freeClientSocketPool;


static int sock2_read_from_client(ClientSocketMap *clientSocketMap);
static void sock2_destroy_socket(void);


int
sock2_init(char* bind_addr, int bind_port)
{
	int i;

	debug(RPT_DEBUG, "%s(bind_addr=\"%s\", port=%d)", __FUNCTION__, bind_addr, bind_port);

	/* create the socket */
	listening_fd = sock2_create_inet_socket(bind_addr, bind_port);
	if (listening_fd < 0) {
		report(RPT_ERR, "%s: error creating socket - %s",
			__FUNCTION__, sock_geterror());
		return -1;
	}

	freeClientSocketPool = (ClientSocketMap *)
				xcalloc(FD_SETSIZE, sizeof(ClientSocketMap), __FUNCTION__);
	if (freeClientSocketPool == NULL) {
		report(RPT_ERR, "%s: Error allocating client sockets.",
			__FUNCTION__);
		return -1;
	}

	freeClientSocketQueue = queue_create();
	if (!freeClientSocketQueue) {
		report(RPT_ERR, "%s: error allocating free socket list.",
			 __FUNCTION__);
		return -1;
	}

	for (i = 0; i < FD_SETSIZE; ++i) {
		queue_insert_node(freeClientSocketQueue, (void*) &freeClientSocketPool[i]);
	}

	openSocketQueue = queue_create();
	if (openSocketQueue == NULL) {
		report(RPT_ERR, "%s: error allocating open socket list.",
			 __FUNCTION__);
		return -1;
	}
	else {
		ClientSocketMap *entry;

		entry = (ClientSocketMap*) queue_pop(freeClientSocketQueue);
		entry->socket = listening_fd;
		entry->client = NULL;
		queue_add_node(openSocketQueue, (void*) entry);
	}

	if ((messageRing = ring_create(MAXMSG)) == NULL) {
		report(RPT_ERR, "%s: error allocating receive buffer.",
			 __FUNCTION__);
		return -1;
	}

	return 0;
}

int
sock2_shutdown(void)
{
	int retVal = 0;

	debug(RPT_DEBUG, "%s()", __FUNCTION__);

	close(listening_fd);
	queue_destroy(freeClientSocketQueue);
	free(freeClientSocketPool);
	ring_destroy(messageRing);

	return retVal;
}


int
sock2_create_inet_socket(char *addr, unsigned int port)
{
	struct sockaddr_in name;
	int sock;
	int sockopt = 1;

	debug(RPT_DEBUG, "%s(addr=\"%s\", port=%i)", __FUNCTION__, addr, port);

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		report(RPT_ERR, "%s: cannot create socket - %s",
			__FUNCTION__, sock_geterror());
		return -1;
	}

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &sockopt, sizeof(sockopt)) < 0) {
		report(RPT_ERR, "%s: error setting socket option SO_REUSEADDR - %s",
			__FUNCTION__, sock_geterror());
		return -1;
	}


	memset(&name, 0, sizeof(name));
	name.sin_family = AF_INET;
	name.sin_port = htons(port);
	inet_aton(addr, &name.sin_addr);

	if (bind(sock, (struct sockaddr *) &name, sizeof(name)) < 0) {
		report(RPT_ERR, "%s: cannot bind to port %d at address %s - %s",
                       __FUNCTION__, port, addr, sock_geterror());
		return -1;
	}

	if (listen(sock, 1) < 0) {
		report(RPT_ERR, "%s: error in attempting to listen to port "
			"%d at %s - %s",
			__FUNCTION__, port, addr, sock_geterror());
		return -1;
	}

	report(RPT_NOTICE, "Listening for queries on %s:%d", addr, port);

	FD_ZERO(&active_fd_set);
	FD_SET(sock, &active_fd_set);

	return sock;
}


int
sock2_poll_clients(void)
{
	struct timeval t;
	ClientSocketMap* clientSocket;

	debug(RPT_DEBUG, "%s()", __FUNCTION__);

	t.tv_sec = 0;
	t.tv_usec = 0;

	read_fd_set = active_fd_set;

	if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, &t) < 0) {
		report(RPT_ERR, "%s: Select error - %s",
			__FUNCTION__, sock_geterror());
		return -1;
	}

	queue_rewind(openSocketQueue);
	for (clientSocket = (ClientSocketMap *) queue_get(openSocketQueue);
	     clientSocket != NULL;
	     clientSocket = queue_get_next(openSocketQueue)) {

		if (FD_ISSET(clientSocket->socket, &read_fd_set)) {
			if (clientSocket->socket == listening_fd) {
				Client *c;
				int new_sock;
				struct sockaddr_in clientname;
				socklen_t size = sizeof(clientname);

				new_sock = accept(listening_fd, (struct sockaddr *) &clientname, &size);
				if (new_sock < 0) {
					report(RPT_ERR, "%s: Accept error - %s",
						__FUNCTION__, sock_geterror());
					return -1;
				}
				report(RPT_NOTICE, "Connect from host %s:%hu on socket %i",
					inet_ntoa(clientname.sin_addr), ntohs(clientname.sin_port), new_sock);
				FD_SET(new_sock, &active_fd_set);

				fcntl(new_sock, F_SETFL, O_NONBLOCK);

				if ((c = client_create(new_sock)) == NULL) {
					report(RPT_ERR, "%s: Error creating client on socket %i - %s",
						__FUNCTION__, clientSocket->socket, sock_geterror());
					return -1;
				}
				else {
					ClientSocketMap *newClientSocket;
					newClientSocket = (ClientSocketMap *) queue_pop(freeClientSocketQueue);
					if (newClientSocket != NULL) {
						newClientSocket->socket = new_sock;
						newClientSocket->client = c;
						queue_insert_node(openSocketQueue, (void *) newClientSocket);
						queue_next(openSocketQueue);
					}
					else {
						report(RPT_ERR, "%s: Error - free client socket list exhausted - %d clients.",
							__FUNCTION__, FD_SETSIZE);
						return -1;
					}
				}
				if (clients_add_client(c) == NULL) {
					report(RPT_ERR, "%s: Could not add client on socket %i",
						 __FUNCTION__, clientSocket->socket);
					return -1;
				}
			}
			else {
				int err = 0;
				debug(RPT_DEBUG, "%s: reading...", __FUNCTION__);
				err = sock2_read_from_client(clientSocket);
				debug(RPT_DEBUG, "%s: ...done", __FUNCTION__);
				if (err < 0)
					sock2_destroy_socket();
			}
		}
	}
	return 0;
}


static int
sock2_read_from_client(ClientSocketMap *clientSocketMap)
{
	char buffer[MAXMSG];
	int nbytes;

	debug(RPT_DEBUG, "%s()", __FUNCTION__);

	errno = 0;
	nbytes = sock2_recv(clientSocketMap->socket, buffer, MAXMSG);

	while (nbytes > 0) {  /* some data is available */
		int fr;
		char *str;

		debug(RPT_DEBUG, "%s: received %4d bytes", __FUNCTION__, nbytes);

		/* append to the ring buffer */
		ring_write(messageRing, buffer, nbytes);

		do {
			str = ring_read_string(messageRing);
			if (clientSocketMap->client) {
				client_add_message(clientSocketMap->client, str);
			} else {
				report(RPT_DEBUG, "%s: Can't find client %d",
					__FUNCTION__, clientSocketMap->socket);
			}
		} while (str != NULL);

		fr = ring_get_max_write(messageRing);
		if (fr == 0)
			report(RPT_WARNING, "%s: Message buffer full", __FUNCTION__);

		nbytes = sock2_recv(clientSocketMap->socket, buffer, min(MAXMSG, fr));
	}

	if (ring_get_max_read(messageRing) > 0) {
		report(RPT_WARNING, "%s: left over bytes in message buffer",
			__FUNCTION__);
		ring_clear(messageRing);
	}

	if (nbytes < 0 && errno == EAGAIN)
		return 0;

	return -1;
}


int byClient(void *csm, void *client)
{
	return (((ClientSocketMap *) csm)->client == (Client *) client) ? 0 : -1;
}


int
sock2_destroy_client_socket(Client *client)
{
	ClientSocketMap *entry;

	queue_rewind(openSocketQueue);
	entry = queue_find(openSocketQueue, byClient, client);

	if (entry != NULL) {
		sock2_destroy_socket();
		return 0;
	}
	return -1;
}

int
sock2_close(int fd)
{
	int err;

	if (fd == SOCK_UNSET)
		return 0;

	err = shutdown(fd, SHUT_RDWR);
	if (!err)
		close(fd);

	return err;
}

static void
sock2_destroy_socket(void)
{
	ClientSocketMap *entry = queue_get(openSocketQueue);

	if (entry != NULL) {
		if (entry->client != NULL) {
			report(RPT_NOTICE, "Client on socket %i disconnected",
				entry->socket);
			client_destroy(entry->client);
			clients_remove_client(entry->client, QD_PREV);
			entry->client = NULL;
		}
		else {
			report(RPT_ERR, "%s: Can't find client of socket %i",
				__FUNCTION__, entry->socket);
		}
		FD_CLR(entry->socket, &active_fd_set);
		close(entry->socket);

		entry = (ClientSocketMap *) queue_delete_node(openSocketQueue, QD_PREV);
		queue_push(freeClientSocketQueue, (void*) entry);
	}
}



typedef struct sockaddr_in sockaddr_in;

static int
sock2_init_sockaddr(sockaddr_in *name, const char *hostname, unsigned short int port)
{
	struct hostent *hostinfo;

	memset (name, '\0', sizeof (*name));
	name->sin_family = AF_INET;
	name->sin_port = htons (port);
	hostinfo = gethostbyname (hostname);
	if (hostinfo == NULL) {
		report (RPT_ERR, "sock2_init_sockaddr: Unknown host %s.", hostname);
		return -1;
	}
	name->sin_addr = *(struct in_addr *) hostinfo->h_addr;

	return 0;
}


int
sock2_send_string(int fd, char *string)
{
	return sock2_send(fd, string, strlen(string));
}


int
sock2_recv_string(int fd, char *dest, size_t maxlen)
{
	char *ptr = dest;
	int recvBytes = 0;

	if (!dest)
		return -1;
	if (maxlen <= 0)
		return 0;

	while (1) {
		int err = read (fd, ptr, 1);
		if (err == -1) {
			if (errno == EAGAIN) {
				if (recvBytes) {
					continue;
				}
				return 0;
			} else {
				report (RPT_ERR, "sock2_recv_string: socket read error");
				return err;
			}
		} else if (err == 0) {
			return recvBytes;
		}

		recvBytes++;

		if (recvBytes == maxlen || *ptr == '\0' || *ptr == '\n') {
			*ptr = '\0';
			break;
		}
		ptr++;
	}

	if (recvBytes == 1 && dest[0] == '\0')
		return 0;

	if (recvBytes < maxlen - 1)
		dest[recvBytes] = '\0';

	return recvBytes;
}


int
sock2_send(int fd, void *src, size_t size)
{
	int offset = 0;

	if (!src)
		return -1;

	while (offset != size) {
                int sent = write (fd, ((char *) src) + offset, size - offset);
		if (sent == -1) {
			if (errno != EAGAIN) {
				report (RPT_ERR, "sock2_send: socket write error");
				report (RPT_DEBUG, "Message was: '%.*s'", size-offset, (char *) src);
				return sent;
			}
			continue;
		} else if (sent == 0) {
			return sent + offset;
		}

		offset += sent;
	}

	return offset;
}

int
sock2_recv(int fd, void *dest, size_t maxlen)
{
	int err;

	if (!dest)
		return -1;
	if (maxlen <= 0)
		return 0;

	err = read (fd, dest, maxlen);
	if (err < 0) {
		return err;
	}

	return err;
}
