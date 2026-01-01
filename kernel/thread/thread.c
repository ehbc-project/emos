#include <emos/thread.h>

#include <stdlib.h>

#include <emos/asm/thread.h>

#include <emos/panic.h>
#include <emos/log.h>
#include <emos/scheduler.h>

#define MODULE_NAME "thread"

static volatile int preemption_enabled = 0;

status_t thread_init(struct thread **main_thread)
{
    status_t status;
    struct thread *main_th = NULL;
    int added_thread_to_scheduler = 0;

    main_th = calloc(1, sizeof(*main_th));
    if (!main_th) {
        status = STATUS_UNKNOWN_ERROR;
        goto has_error;
    }
    main_th->id = 0;
    main_th->status = TS_RUNNING;

    status = scheduler_add_thread(main_th);
    if (!CHECK_SUCCESS(status)) goto has_error;
    added_thread_to_scheduler = 1;

    status = scheduler_set_current_thread(main_th);
    if (!CHECK_SUCCESS(status)) goto has_error;

    if (main_thread) *main_thread = main_th;
    
    return STATUS_SUCCESS;

has_error:
    if (added_thread_to_scheduler) {
        scheduler_remove_thread(main_th);
    }

    if (main_th) {
        free(main_th);
    }

    return status;
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
    static int new_thread_id = 1;

    status_t status;
    int prev_preemption_enabled = preemption_enabled;
    struct thread *th = NULL;
    int added_thread_to_scheduler = 0;
    void *stack_base = NULL, *stack_ptr = NULL;

    thread_stop_preemption();

    /* create thread object */
    th = calloc(1, sizeof(*th));
    if (!th) {
        status = STATUS_UNKNOWN_ERROR;
        goto has_error;
    }
    th->id = new_thread_id++;

    /* prepare stack */
    status = thread_prepare_stack(th, stack_size, entry, &stack_base, &stack_ptr);
    if (!CHECK_SUCCESS(status)) goto has_error;

    th->stack_base = stack_base;
    th->stack_ptr = stack_ptr;
    th->stack_size = stack_size;
    th->entry = entry;
        
    /* add thread object to list */
    status = scheduler_add_thread(th);
    if (!CHECK_SUCCESS(status)) goto has_error;
    added_thread_to_scheduler = 1;

    LOG_DEBUG("created thread #%d\n", th->id);

    if (threadout) *threadout = th;

    if (prev_preemption_enabled) {
        thread_start_preemption();
    }

    return STATUS_SUCCESS;

has_error:
    if (added_thread_to_scheduler) {
        scheduler_remove_thread(th);
    }

    if (stack_ptr) {
        free(stack_ptr);
    }

    if (th) {
        free(th);
    }

    if (prev_preemption_enabled) {
        thread_start_preemption();
    }

    return status;
}

status_t thread_remove(struct thread *th)
{
    if (!th->entry) return STATUS_INVALID_THREAD;
    if (th->status != TS_FINISHED) return STATUS_THREAD_NOT_FINISHED;

    LOG_DEBUG("removing thread #%d\n", th->id);

    scheduler_remove_thread(th);

    free(th->stack_base);

    free(th);

    return STATUS_SUCCESS;
}

status_t thread_wait(struct thread **list, int count, int timeout)
{
    status_t status;
    struct thread *current_thread;
    
    status = scheduler_get_current_thread(&current_thread);
    if (!CHECK_SUCCESS(status)) return status;

    current_thread->wait_list = list;
    current_thread->wait_count = count;
    current_thread->wait_timeout = timeout;
    current_thread->status = TS_WAITING;

    scheduler_yield();

    return STATUS_SUCCESS;
}
