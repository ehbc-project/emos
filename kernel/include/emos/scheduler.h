#ifndef __EMOS_SCHEDULER_H__
#define __EMOS_SCHEDULER_H__

#include <emos/thread.h>

status_t scheduler_add_thread(struct thread *th);
void scheduler_remove_thread(struct thread *th);

status_t scheduler_get_current_thread(struct thread **current);
status_t scheduler_get_next_thread(struct thread **next);
status_t scheduler_set_current_thread(struct thread *th);

#endif // __EMOS_SCHEDULER_H__
