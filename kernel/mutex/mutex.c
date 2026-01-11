#include <emos/mutex.h>

#include <emos/log.h>
#include <emos/scheduler.h>
#include <emos/thread.h>

#define MODULE_NAME "mutex"

status_t mutex_init(struct mutex *mtx)
{
    mtx->locked = 0;
    mtx->owner = NULL;
    
    return STATUS_SUCCESS;
}

static void add_blocking_thread(struct mutex *mtx, struct thread *th)
{
    struct thread *last_blocking_th;
        
    LOG_DEBUG("blocking thread #%d\n", th->id);

    if (mtx->blocking_threads == th) return;

    if (!mtx->blocking_threads) {
        mtx->blocking_threads = th;
        th->mutex_blocking_next = NULL;
        return;
    }

    last_blocking_th = mtx->blocking_threads;
    while (last_blocking_th) {
        if (last_blocking_th == th) return;
        if (!last_blocking_th->mutex_blocking_next) break;
        last_blocking_th = last_blocking_th->mutex_blocking_next;
    }

    last_blocking_th->mutex_blocking_next = th;
    th->mutex_blocking_next = NULL;
}

static void unblock_blocking_thread(struct mutex *mtx)
{
    struct thread *th_to_unblock;

    if (!mtx->blocking_threads) return;
    
    th_to_unblock = mtx->blocking_threads;
    mtx->blocking_threads = th_to_unblock->mutex_blocking_next;
        
    LOG_DEBUG("unblocking thread #%d\n", th_to_unblock->id);

    th_to_unblock->mutex_blocking_next = NULL;
    if (th_to_unblock->status == TS_BLOCKING) {
        th_to_unblock->status = TS_RUNNING;
    }
}

status_t mutex_lock(struct mutex *mtx)
{
    status_t status;
    struct thread *th;

    status = scheduler_get_current_thread(&th);
    if (!CHECK_SUCCESS(status)) return status;

    thread_disable_preemption();

    while (mtx->locked) {
        thread_enable_preemption();

        th->status = TS_BLOCKING;

        add_blocking_thread(mtx, th);

        scheduler_yield();

        thread_disable_preemption();
    }

    mtx->locked = 1;
    mtx->owner = th;

    thread_enable_preemption();

    return STATUS_SUCCESS;
}

status_t mutex_try_lock(struct mutex *mtx)
{
    status_t status;
    struct thread *th;

    status = scheduler_get_current_thread(&th);
    if (!CHECK_SUCCESS(status)) return status;
    
    thread_disable_preemption();

    if (mtx->locked) {
        thread_enable_preemption();

        return STATUS_MUTEX_LOCKED;
    }
    
    mtx->locked = 1;
    mtx->owner = th;

    thread_enable_preemption();

    return STATUS_SUCCESS;
}

status_t mutex_unlock(struct mutex *mtx)
{
    status_t status;
    struct thread *th;

    status = scheduler_get_current_thread(&th);
    if (!CHECK_SUCCESS(status)) return status;

    thread_disable_preemption();

    if (mtx->owner != th) {
        thread_enable_preemption();

        return STATUS_INVALID_THREAD;
    }

    mtx->locked = 0;
    mtx->owner = NULL;

    unblock_blocking_thread(mtx);
    
    thread_enable_preemption();

    return STATUS_SUCCESS;
}
