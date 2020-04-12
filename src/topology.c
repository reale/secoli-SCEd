#include "node.h"
#include "topology.h"
#include "utils.h"  /* bool */
#include "xalloc.h"


static Topology *topology = NULL;
static bool we_are_leader = false;


bool
topology_init(void)
{
	topology = xmalloc(sizeof(Topology), __FUNCTION__);
	if (!topology)
		return false;

	topology->nodes = queue_create();
	if (!topology->nodes) {
		topology_destroy();
		return false;
	}

	we_are_leader = true;

	return true;
}

void
topology_destroy(void)
{
	if (topology) {
		if (topology->nodes)
			queue_destroy(topology->nodes);

		free(topology);
	}
}

bool
topology_announce_node(Node *node)
{
	if (!node || !we_are_leader)
		return false;

	queue_push(topology->nodes, node);

	return true;
}

bool
topology_leader(void)
{
	return we_are_leader;
}

int
topology_count_nodes(void)
{
	if (!we_are_leader)
		return -1;
	
	return queue_get_length(topology->nodes);
}

char *
topology_list_nodes(int index)
{
	Node *node;

	if (!we_are_leader)
		return NULL;

	if (index == 0)
		node = queue_get_first(topology->nodes);
	else
		node = queue_get_next(topology->nodes);

	return node_pretty_print(node);
}
