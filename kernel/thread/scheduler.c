#include <emos/scheduler.h>

#include <emos/panic.h>

static struct thread *volatile first_thread = NULL;
static struct thread *volatile current_thread = NULL;

status_t scheduler_add_thread(struct thread *th)
{
    th->next = first_thread;
    first_thread = th;

    return STATUS_SUCCESS;
}

status_t scheduler_remove_thread(struct thread *th)
{
    if (th->status != TS_FINISHED) {
        return STATUS_THREAD_NOT_FINISHED;
    }

    if (th == first_thread) {
        current_thread = th->next;
    }

    for (struct thread *current = first_thread; current->next; current = current->next) {
        if (th == current->next) {
            current->next = th->next;
            break;
        }
    }

    return STATUS_SUCCESS;
}

status_t scheduler_get_current_thread(struct thread **current)
{
    if (current) *current = current_thread;

    return STATUS_SUCCESS;
}

status_t scheduler_get_next_thread(struct thread **next)
{
    struct thread *next_thread = current_thread;
    
    do {
        next_thread = next_thread->next;
        if (!next_thread) {
            next_thread = first_thread;
        }
    } while (next_thread->status != TS_RUNNING && next_thread->status != TS_PENDING);
    
    if (next) *next = next_thread;

    return STATUS_SUCCESS;
}

status_t scheduler_set_current_thread(struct thread *th)
{
    current_thread = th;

    return STATUS_SUCCESS;
}

int scheduler_has_other_runnable_thread(void)
{
    int result = 0;

    for (struct thread *current = first_thread; current; current = current->next) {
        if (current == current_thread) continue;

        if (current->status == TS_RUNNING || current->status == TS_PENDING) {
            result = 1;
        } 
    }

    return result;
}

status_t scheduler_yield(void)
{
    asm volatile (
        "pushf\n\t"
        "cli\n\t"
        "int $0x20\n\t"
        "popf\n\t"
    );

    return STATUS_SUCCESS;
}

status_t scheduler_maintain(void)
{
    int unwait_thread;

    if (current_thread && current_thread->entry) {
        /* if there's an entry point, then it's not a main thread */
        return STATUS_INVALID_THREAD;
    }

    for (struct thread *current = first_thread; current; current = current->next) {
        if (current->status != TS_WAITING) continue;
        if (!current->wait_list) continue;

        unwait_thread = 1;

        for (int i = 0; i < current->wait_count; i++) {
            if (!current->wait_list[i]) continue;
            if (current->wait_list[i]->status != TS_FINISHED) {
                unwait_thread = 0;
                break;
            }

            current->wait_list[i] = NULL;
        }

        if (unwait_thread) {
            current->wait_list = NULL;
            current->status = TS_RUNNING;
        }
    }

    struct thread *current, *prev;
    prev = NULL;
    current = first_thread;
    while (current) {
        if (current->status != TS_FINISHED) {
            prev = current;
            current = current->next;
            continue;
        }
        
        struct thread *thread_to_remove = current;
        
        if (prev) {
            prev->next = current->next;
        } else {
            first_thread = current->next;
        }

        current = current->next;

        thread_remove(thread_to_remove);
    }

    return STATUS_SUCCESS;
}
