#ifndef DAEMON_H
#define DAEMON_H

pid_t daemon_fork(void);
int daemon_say_ok_to_parent(pid_t parent_pid);

#endif /* DAEMON_H */
