/* socket internals */

#ifndef SOCK2_H
#define SOCK2_H

#include "client.h"

#ifndef SHUT_RDWR
# define SHUT_RDWR 2
#endif


int sock2_init(char* bind_addr, int bind_port);
int sock2_shutdown(void);
int sock2_create_inet_socket(char* bind_addr, unsigned int port);
int sock2_poll_clients(void);
int sock2_destroy_client_socket(Client *client);

int sock2_close(int fd);

int sock2_recv_string(int fd, char *dest, size_t maxlen);
int sock2_recv(int fd, void *dest, size_t maxlen);

int sock2_send(int fd, void *src, size_t size);
int sock2_send_string(int fd, char *string);

#endif  /* SOCK2_H */
