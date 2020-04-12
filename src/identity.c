#include <uuid/uuid.h>

#include "identity.h"
#include "xalloc.h"


static uuid_t server_identity;


void
identity_init(void)
{
	uuid_generate(server_identity);
}

char *
identity_get(void)
{
	/* TODO:  avoid duplicating logic in client_get_identity() */

	char *identity_unparsed;

	identity_unparsed = (char *) xmalloc(37, __FUNCTION__);  /* 36 + '\0' */

	uuid_unparse_upper(server_identity, identity_unparsed);

	return identity_unparsed;
}
