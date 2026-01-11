#ifndef __EMOS_ASM_THREAD_H__
#define __EMOS_ASM_THREAD_H__

#include <emos/thread.h>

status_t _pc_thread_allocate_kthread_stack(struct thread *th);
status_t _pc_thread_setup_kthread_stack(struct thread *th);
void _pc_thread_free_kthread_stack(struct thread *th);

#define thread_allocate_kthread_stack _pc_thread_allocate_kthread_stack
#define thread_setup_kthread_stack _pc_thread_setup_kthread_stack
#define thread_free_kthread_stack _pc_thread_free_kthread_stack

#endif // __EMOS_ASM_THREAD_H__
