#include <emos/thread.h>

#include <stdlib.h>

#include <emos/asm/thread.h>

#include <emos/panic.h>
#include <emos/scheduler.h>

static volatile int preemption_enabled = 0;

void thread_init(void)
{

}

void thread_start_preemption(void)
{
    preemption_enabled = 1;
}

void thread_stop_preemption(void)
{
    preemption_enabled = 0;
}

int thread_is_preemption_enabled(void)
{
    return preemption_enabled;
}

status_t thread_create(thread_entry_t entry, size_t stack_size, struct thread **threadout)
{
    static int new_thread_id = 0;

    status_t status;
    struct thread *th = NULL;
    int added_thread_to_scheduler = 0;
    int prev_preemption_enabled = preemption_enabled;

    thread_stop_preemption();

    /* create thread object */
    th = calloc(1, sizeof(*th));
    if (!th) {
        status = STATUS_UNKNOWN_ERROR;
        goto has_error;
    }
    th->id = new_thread_id++;

    /* add thread object to list */
    status = scheduler_add_thread(th);
    if (!CHECK_SUCCESS(status)) goto has_error;
    added_thread_to_scheduler = 1;

    if (!entry) {
        /* main thread */
        status = scheduler_set_current_thread(th);
        if (!CHECK_SUCCESS(status)) goto has_error;

        th->status = 1;
    } else  {
        status = thread_prepare_stack(th, stack_size, entry, &th->stack_base, &th->stack_ptr);
        if (!CHECK_SUCCESS(status)) goto has_error;

        th->stack_size = stack_size;
        th->entry = entry;
    }

    if (threadout) *threadout = th;

    if (prev_preemption_enabled) {
        thread_start_preemption();
    }

    return STATUS_SUCCESS;

has_error:
    if (added_thread_to_scheduler) {
        scheduler_remove_thread(th);
    }

    if (th) {
        free(th);
    }

    if (prev_preemption_enabled) {
        thread_start_preemption();
    }

    return status;
}
