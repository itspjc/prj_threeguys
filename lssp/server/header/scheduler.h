
#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "header/socket.h"

void schedule_init();
u_long schedule_enter(u_long time, u_long session_id, void (*fun)(void*), void *arg);
void schedule_remove_session(u_long session_id);
void schedule_execute(u_long time);
int schedule_check( u_long session_id );

#endif  /* _SCHEDULER_H_ */
