#include <emos/asm/thread.h>

#include <stdlib.h>
#include <string.h>

#include <emos/asm/pc_gdt.h>
#include <emos/asm/isr.h>
#include <emos/asm/intrinsics/misc.h>
#include <emos/asm/page.h>

#include <emos/log.h>
#include <emos/panic.h>
#include <emos/scheduler.h>
#include <emos/thread.h>
#include <emos/mm.h>

#define MODULE_NAME "asm_thread"

__noreturn
static void real_kernel_thread_entry(void)
{
    thread_entry_t entry;
    struct thread *th;

    asm volatile (
        "mov %%ebx, %0\n\t"
        "mov %%ecx, %1\n\t"
        : "=m"(th), "=m"(entry)
    );

    entry(th);

    thread_exit();
}

status_t _pc_thread_allocate_kthread_stack(struct thread *th)
{
    status_t status;
    pfn_t kmode_stack_base_pfn = 0;
    vpn_t kmode_stack_base_vpn = 0;
    int kmode_stack_mapped = 0;

    /* allocate thread stack */
    // TODO: scattered stack pfn allocation
    status = mm_pma_allocate_frame(th->kmode_stack_page_count, &kmode_stack_base_pfn, PAF_DEFAULT);
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = mm_vma_allocate_page(th->kmode_stack_page_count, &kmode_stack_base_vpn, VAF_KERNEL);
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = mm_map(kmode_stack_base_pfn, kmode_stack_base_vpn, th->kmode_stack_page_count, PMF_DEFAULT);
    if (!CHECK_SUCCESS(status)) goto has_error;
    kmode_stack_mapped = 1;

    th->kmode_stack_base_vpn = kmode_stack_base_vpn * PAGE_SIZE;
    th->kmode_stack_ptr = (void *)((kmode_stack_base_vpn + th->kmode_stack_page_count) * PAGE_SIZE);

    return STATUS_SUCCESS;

has_error:
    if (kmode_stack_mapped) {
        mm_unmap(kmode_stack_base_vpn, th->kmode_stack_page_count);
    }

    if (kmode_stack_base_vpn) {
        mm_vma_free_page(kmode_stack_base_vpn, th->kmode_stack_page_count);
    }

    if (kmode_stack_base_pfn) {
        mm_pma_free_frame(kmode_stack_base_pfn, th->kmode_stack_page_count);
    }

    return status;
}

status_t _pc_thread_setup_kthread_stack(struct thread *th)
{
    uintptr_t esp;
    struct isr_regs *iregs;
    struct interrupt_frame *iframe;

    esp = (uintptr_t)th->kmode_stack_ptr;

    /* fill initial interrupt stack frame */
    esp -= 16;
    iframe = (void *)esp;
    memset(iframe, 0, sizeof(*iframe));
    iframe->eflags = 0x00000202;
    iframe->cs = SEG_SEL_KERNEL_CODE;
    iframe->eip = (uintptr_t)real_kernel_thread_entry;

    /* fill initial register stack */
    esp -= sizeof(*iregs);
    iregs = (void *)esp;
    memset(iregs, 0, sizeof(*iregs));
    iregs->ebx = (uintptr_t)th;
    iregs->ecx = (uintptr_t)th->kmode_entry;
    iregs->ds = SEG_SEL_KERNEL_DATA;
    iregs->es = SEG_SEL_KERNEL_DATA;
    iregs->fs = SEG_SEL_KERNEL_DATA;
    iregs->gs = SEG_SEL_KERNEL_DATA;

    /* stack area for dummy ebp (only for debugging purpose, will be replaced by popal) */
    esp -= 4;

    th->kmode_stack_ptr = (void *)esp;

    return STATUS_SUCCESS;
}

void _pc_thread_free_kthread_stack(struct thread *th)
{
    status_t status;
    pfn_t kmode_stack_base_pfn;

    LOG_DEBUG("freeing thread stack\n");

    status = mm_vpn_to_pfn(th->kmode_stack_base_vpn, &kmode_stack_base_pfn);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "failed to get kmode_stack_base page mapping");
    }

    status = mm_unmap(th->kmode_stack_base_vpn, th->kmode_stack_page_count);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "failed to unmap kmode_stack_base");
    }

    mm_vma_free_page(th->kmode_stack_base_vpn, th->kmode_stack_page_count);
    mm_pma_free_frame(kmode_stack_base_pfn, th->kmode_stack_page_count);
}
