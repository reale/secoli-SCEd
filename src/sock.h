#ifndef SOCK_H
#define SOCK_H

#include "client.h"

#define SOCK_UNSET -1

/* Max length of messages sent by sock_printf() */
#define SOCK_MAX_BUFLEN 8192

#define SOCK_ANSWER_OK "OK"
#define SOCK_ANSWER_UNDEFINED "UNDEFINED"
#define SOCK_ANSWER_UNAVAILABLE "UNAVAILABLE"
#define SOCK_ANSWER_UNIMPLEMENTED "UNIMPLEMENTED"
#define SOCK_ANSWER_HELLO "SCE GATEWAY VERSION %s PROTOCOL VERSION %s\n"


/* Server functions...*/
int sock_init(char* bind_addr, int bind_port);
int sock_shutdown(void);
int sock_create_inet_socket(char* bind_addr, unsigned int port);
int sock_poll_clients(void);
int sock_destroy_client_socket(Client *client);
int sock_close(int fd);

/* Receive raw data */
int sock_recv(int fd, void *dest, size_t maxlen);

/* Receive a line of text */
int sock_recv_string(int fd, char *dest, size_t maxlen);

/* Send raw data */
int sock_send(int fd, void *src, size_t size);

/* Send a line of text */
int sock_send_string(int fd, char *string);

int sock_printf(int fd, const char *format, .../*args*/);
int sock_printf_huge(int fd, const char *format, .../*args*/);

int sock_answer_ok(int fd);
int sock_answer_undefined(int fd);
int sock_answer_unavailable(int fd);
int sock_answer_unimplemented(int fd);

char *sock_geterror(void);

#endif  /* SOCK_H */
