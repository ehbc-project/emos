#include <emos/thread.h>

#include <stdlib.h>

#include <emos/asm/thread.h>
#include <emos/asm/page.h>

#include <emos/panic.h>
#include <emos/log.h>
#include <emos/scheduler.h>
#include <emos/macros.h>

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
    main_th->type = TT_MAIN;

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

void thread_enable_preemption(void)
{
    preemption_enabled = 1;
}

void thread_disable_preemption(void)
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
    int stack_allocated = 0;
    int added_thread_to_scheduler = 0;

    thread_disable_preemption();

    /* create thread object */
    th = calloc(1, sizeof(*th));
    if (!th) {
        status = STATUS_UNKNOWN_ERROR;
        goto has_error;
    }
    th->id = new_thread_id++;
    th->status = TS_PENDING;
    th->type = TT_KERNEL;

    /* prepare stack */
    th->kmode_stack_page_count = ALIGN_DIV(stack_size, PAGE_SIZE);
    th->kmode_entry = entry;

    status = thread_allocate_kthread_stack(th);
    if (!CHECK_SUCCESS(status)) goto has_error;
    stack_allocated = 1;

    status = thread_setup_kthread_stack(th);
    if (!CHECK_SUCCESS(status)) goto has_error;

    /* add thread object to list */
    status = scheduler_add_thread(th);
    if (!CHECK_SUCCESS(status)) goto has_error;
    added_thread_to_scheduler = 1;

    LOG_DEBUG("created thread #%d\n", th->id);

    if (threadout) *threadout = th;

    if (prev_preemption_enabled) {
        thread_enable_preemption();
    }

    return STATUS_SUCCESS;

has_error:
    if (th && added_thread_to_scheduler) {
        scheduler_remove_thread(th);
    }

    if (th && stack_allocated) {
        thread_free_kthread_stack(th);
    }

    if (th) {
        free(th);
    }

    if (prev_preemption_enabled) {
        thread_enable_preemption();
    }

    return status;
}

status_t thread_remove(struct thread *th)
{
    if (th->type == TT_MAIN) return STATUS_INVALID_THREAD;
    if (th->status != TS_FINISHED) return STATUS_THREAD_NOT_FINISHED;

    LOG_DEBUG("removing thread #%d\n", th->id);

    scheduler_remove_thread(th);

    thread_free_kthread_stack(th);

    free(th);

    return STATUS_SUCCESS;
}

status_t thread_detach(struct thread *thread)
{
    status_t status;
    struct thread *current_thread;
    
    status = scheduler_get_current_thread(&current_thread);
    if (!CHECK_SUCCESS(status)) return status;

    if (thread->wait_list) return STATUS_CONFLICTING_STATE;

    thread->detached = 1;

    LOG_DEBUG("detaching thread #%d\n", thread->id);

    return STATUS_SUCCESS;
}

status_t thread_wait(struct thread **list, int count, int timeout)
{
    status_t status;
    struct thread *current_thread;
    
    status = scheduler_get_current_thread(&current_thread);
    if (!CHECK_SUCCESS(status)) return status;

    for (int i = 0; i < count; i++) {
        if (list[i]->detached) return STATUS_CONFLICTING_STATE;
    }

    thread_disable_preemption();

    // TODO: implement timeout

    current_thread->wait_list = list;
    current_thread->wait_count = count;
    current_thread->wait_timeout = timeout;
    current_thread->status = TS_WAITING;

    thread_enable_preemption();

    scheduler_yield();

    return STATUS_SUCCESS;
}

__noreturn
void thread_exit(void)
{
    status_t status;
    struct thread *current_thread;
    
    status = scheduler_get_current_thread(&current_thread);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "cannot get current thread");
    }

    if (current_thread->type == TT_MAIN) {
        panic(STATUS_INVALID_THREAD, "cannot exit from main thread");
    }

    current_thread->status = TS_FINISHED;

    LOG_DEBUG("thread #%d finished\n", current_thread->id);

    scheduler_yield();

    for (;;) {}
}
