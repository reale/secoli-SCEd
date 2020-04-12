#ifndef NODE_H
#define NODE_H


typedef struct Node {
	char *address;
	int port;
} Node;

Node *node_assemble(const char *address, const char *port);
void node_destroy(Node *node);

#endif  /* NODE_H */
