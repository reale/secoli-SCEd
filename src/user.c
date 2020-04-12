#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "user.h"
#include "xalloc.h"
#include "xconfig.h"


User *
user_authenticate(const char *name, const char *password)
{
	User *user;
	char *stored_password;

	if (!name || !password)
		return NULL;
	
	if (!strlen(name) || !strlen(password))
		return NULL;

	stored_password = xconfig_get_string("Users", name, 0, NULL);
	if (!stored_password || strcmp(stored_password, password))
		return NULL;

	user = xmalloc(sizeof(User), __FUNCTION__);

	return user;
}

int
user_deauthenticate(User *user)
{
	if (!user)
		return -1;
	
	user_destroy(user);

	return 0;
}

void
user_destroy(User *user)
{
	/*
	if (user)
		free(user);
	*/
}
