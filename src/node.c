#include "node.h"
#include "xalloc.h"


Node *
node_assemble(const char *address, const char *port)
{
	Node *node;

	node = xmalloc(sizeof(Node), __FUNCTION__);
	if (!node)
		return NULL;

	node->address = strdup(address);
	node->port = atoi(port);

	return node;
}
	
void
node_destroy(Node *node)
{
	if (node) {
		if (node->address)
			free(node->address);
		free(node);
	}
}

char *
node_pretty_print(Node *node)
{
	char *buffer;

	if (!node || !node->address)
		return NULL;

	buffer = xmalloc(32, __FUNCTION__);
	if (!buffer)
		return NULL;

	snprintf(buffer, 32, "%s:%d", node->address, node->port);
	return buffer;
}
