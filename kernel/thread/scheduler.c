#include <emos/scheduler.h>

static struct thread *volatile thread_first_th = NULL;
static struct thread *volatile thread_current_th = NULL;

status_t scheduler_add_thread(struct thread *th)
{
    th->next = thread_first_th;
    thread_first_th = th;

    return STATUS_SUCCESS;
}

void scheduler_remove_thread(struct thread *th)
{
    if (th == thread_first_th) {
        thread_current_th = th->next;
    }

    for (struct thread *current = thread_first_th; current->next; current = current->next) {
        if (th == current->next) {
            current->next = th->next;
            break;
        }
    }
}

status_t scheduler_get_current_thread(struct thread **current)
{
    if (current) *current = thread_current_th;

    return STATUS_SUCCESS;
}

status_t scheduler_get_next_thread(struct thread **next)
{
    struct thread *next_thread = thread_current_th->next;
    if (!next_thread) {
        next_thread = thread_first_th;
    }
    
    if (next) *next = next_thread;

    return STATUS_SUCCESS;
}

status_t scheduler_set_current_thread(struct thread *th)
{
    thread_current_th = th;

    return STATUS_SUCCESS;
}
