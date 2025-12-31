#include <emos/thread.h>

#include <stdlib.h>

#include <emos/asm/thread.h>
#include <emos/panic.h>

static struct thread *thread_first_th = NULL;
static struct thread *thread_current_th = NULL;
static int thread_enabled = 0;
static int thread_disabling = 0;

void thread_init(void)
{

}

void thread_start(void)
{
    while (thread_disabling) {}  /* spin wait */

    thread_enabled = 1;
}

void thread_stop(void)
{
    panic(STATUS_UNIMPLEMENTED, "NONSTOP");
}

status_t thread_get_current(struct thread **current)
{
    if (!thread_enabled) return STATUS_FEATURE_DISABLED;

    if (current) *current = thread_current_th;

    return STATUS_SUCCESS;
}

status_t thread_get_next_scheduled(struct thread **next)
{
    if (!thread_enabled) return STATUS_FEATURE_DISABLED;

    struct thread *next_thread = thread_current_th->next;
    if (!next_thread) {
        next_thread = thread_first_th;
    }
    
    if (next) *next = next_thread;

    return STATUS_SUCCESS;
}

void thread_switch(struct thread *next)
{
    thread_current_th = next;
}

status_t thread_create(thread_entry_t entry, size_t stack_size, struct thread **threadout)
{
    static int new_thread_id = 0;

    status_t status;
    struct thread *th = NULL;

    /* create thread object */
    th = calloc(1, sizeof(*th));
    if (!th) {
        status = STATUS_UNKNOWN_ERROR;
        goto has_error;
    }
    th->id = new_thread_id++;

    /* add thread object to list */
    th->next = thread_first_th;
    thread_first_th = th;

    if (!entry) {
        /* main thread */
        thread_current_th = th;

        th->running = 1;
    } else  {
        status = thread_prepare_stack(th, stack_size, entry, &th->stack_base, &th->stack_ptr);
        if (!CHECK_SUCCESS(status)) goto has_error;

        th->stack_size = stack_size;
        th->entry = entry;
    }

    if (threadout) *threadout = th;

    return STATUS_SUCCESS;

has_error:
    if (th) {
        free(th);
    }

    return status;
}
