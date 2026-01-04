#ifndef __EMOS_THREAD_H__
#define __EMOS_THREAD_H__

#include <emos/status.h>
#include <emos/compiler.h>

struct thread;

typedef void (*thread_entry_t)(struct thread *);

#define TS_PENDING      0
#define TS_RUNNING      1
#define TS_BLOCKING     2
#define TS_WAITING      3
#define TS_FINISHED     4

struct thread {
    struct thread *next;

    int id;

    int status;
    size_t stack_size;
    void *stack_base;
    void *stack_ptr;
    thread_entry_t entry;

    int detached;

    struct thread **wait_list;
    int wait_count;
    int wait_timeout;

    struct thread *mutex_blocking_next;
};

status_t thread_init(struct thread **main_thread);

void thread_enable_preemption(void);
void thread_disable_preemption(void);
int thread_is_preemption_enabled(void);

status_t thread_create(thread_entry_t entry, size_t stack_size, struct thread **threadout);
status_t thread_remove(struct thread *thread);

status_t thread_detach(struct thread *thread);
status_t thread_wait(struct thread **list, int count, int timeout);

__noreturn
void thread_exit(void);

#endif // __EMOS_THREAD_H__
