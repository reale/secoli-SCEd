#ifndef USER_H
#define USER_H

typedef struct User {

} User;

User *user_authenticate(const char *name, const char *password);
int user_deauthenticate(User *user);

void user_destroy(User *user);

#endif  /* USER_H */
