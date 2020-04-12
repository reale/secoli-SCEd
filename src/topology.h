#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include "node.h"
#include "queue.h"
#include "utils.h"  /* bool */


typedef struct Topology {
	Queue *nodes;
} Topology;


bool topology_init(void);
void topology_destroy(void);

bool topology_announce_node(Node *node);
int topology_count_nodes(void);
bool topology_leader(void);
char *topology_list_nodes(int index);

#endif  /* TOPOLOGY_H */
