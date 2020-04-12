#include "client.h"
#include "output_void.h"
#include "search.h"
#include "utils.h"  /* bool */


int
output_void_open(void *params)
{
	return 0;
}

int
output_void_print(Client *c, Search *search, bool print_result_count)
{
	return 0;
}

int
output_void_close(void)
{
	return 0;
}
