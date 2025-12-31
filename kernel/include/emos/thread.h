#ifndef __EMOS_THREAD_H__
#define __EMOS_THREAD_H__

#include <emos/status.h>

struct thread;

typedef void (*thread_entry_t)(struct thread *);

struct thread {
    struct thread *next;

    int id;

    int running;
    size_t stack_size;
    void *stack_base;
    void *stack_ptr;
    thread_entry_t entry;
};

void thread_init(void);

void thread_start_preemption(void);
void thread_stop_preemption(void);
int thread_is_preemption_enabled(void);

status_t thread_create(thread_entry_t entry, size_t stack_size, struct thread **threadout);
status_t thread_wait(struct thread *thread, int timeout);
status_t thread_remove(struct thread *thread);

#endif // __EMOS_THREAD_H__
