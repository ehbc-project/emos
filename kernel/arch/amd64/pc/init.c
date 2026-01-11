#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <emos/asm/gdt.h>
#include <emos/asm/pc_gdt.h>
#include <emos/asm/io.h>
#include <emos/asm/page.h>
#include <emos/asm/isr.h>
#include <emos/asm/pic.h>
#include <emos/asm/instruction.h>

#include <emos/compiler.h>
#include <bootemos/bootinfo.h>
#include <emos/mm.h>
#include <emos/status.h>
#include <emos/macros.h>
#include <emos/panic.h>
#include <emos/log.h>
#include <emos/thread.h>
#include <emos/scheduler.h>

#define MODULE_NAME "init"

int _pc_invlpg_undefined = 1;
int _pc_rdtsc_undefined = 1;

static void invlpg_test(void)
{
    asm volatile ("invlpg (%0)" : : "r"(0));
}

static void rdtsc_test(void)
{
    uint32_t low, high;
    asm volatile ("rdtsc" : "=a"(low), "=d"(high));
}

struct bootinfo_table_header *_pc_bootinfo_table;

static int early_print_char(void *, char ch)
{
    switch (ch) {
        case '\0':
        case '\r':
            return 1;
        default:
            io_out8(0x00E9, ch);
            return 0;
    }
}

void _pc_init(struct bootinfo_table_header *btblhdr)
{
    status_t status;
    struct bootinfo_entry_header *enthdr = NULL;
    struct bootinfo_entry_memory_map *mment = NULL;
    struct bootinfo_entry_unavailable_frames *ufent = NULL;
    struct bootinfo_entry_pagetable_vpn *pvent = NULL;
    struct bootinfo_table_header *newbtblhdr = NULL;

    log_early_init(early_print_char, NULL);

    LOG_DEBUG("Starting kernel...\n");

    enthdr = (void *)((uintptr_t)btblhdr + btblhdr->header_size);
    for (int i = 0; i < btblhdr->entry_count; i++) {
        switch (enthdr->type) {
            case BET_MEMORY_MAP:
                mment = (void *)((uintptr_t)enthdr + enthdr->header_size);
                break;
            case BET_UNAVAILABLE_FRAMES:
                ufent = (void *)((uintptr_t)enthdr + enthdr->header_size);
                break;
            case BET_PAGETABLE_VPN:
                pvent = (void *)((uintptr_t)enthdr + enthdr->header_size);
                break;
            default:
                break;
        }

        enthdr = (void *)((uintptr_t)enthdr + enthdr->size);
    }

    if (!mment || !ufent || !pvent) {
        panic(STATUS_ENTRY_NOT_FOUND, "required entry not found");
    }

    LOG_DEBUG("initializing GDT...\n");
    _pc_gdt_init();

    LOG_DEBUG("initializing physical memory allocator...\n");
    status = mm_pma_init(pvent->vpn, mment, ufent);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "failed to initialize physical memory allocator");
    }

    LOG_DEBUG("initializing virtual memory allocator...\n");
    status = mm_vma_init(0x00000100, 0x000BFFFF, 0x000C1000, 0x000FEFFF);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "failed to initialize virtual memory allocator");
    }

    LOG_DEBUG("initializing memory management...\n");
    status = mm_init();
    if (!CHECK_SUCCESS(status)) {
        panic(status, "failed to initialize memory management");
    }

    LOG_DEBUG("relocating bootinfo table...\n");
    newbtblhdr = malloc(btblhdr->size);
    if (!newbtblhdr) {
        panic(status, "cannot allocate memory for bootinfo table");
    }
    
    memcpy(newbtblhdr, btblhdr, btblhdr->size);
    
    _pc_bootinfo_table = newbtblhdr;

    LOG_DEBUG("%p %p %p %08lX\n", (void *)_pc_bootinfo_table, (void *)btblhdr, (void *)enthdr, btblhdr->size);
}

static volatile uint64_t global_tick = 0;

uint64_t get_global_tick(void)
{
    return global_tick;
}

static void *switch_thread(struct interrupt_frame *frame, struct isr_regs *regs)
{
    status_t status;
    struct thread *current_thread, *next_thread;

    status = scheduler_get_current_thread(&current_thread);
    if (!CHECK_SUCCESS(status)) return NULL;

    /* ask to scheduler */
    status = scheduler_get_next_thread(&next_thread);
    if (!CHECK_SUCCESS(status) || !next_thread) return NULL;

    /* check thread status */
    switch (next_thread->status) {
        case TS_PENDING:
            next_thread->status = TS_RUNNING;
            break;
        case TS_RUNNING:
            break;
        case TS_BLOCKING:
            break;
        case TS_FINISHED:
            // might be a scheduler error
            break;
        default:
            panic(STATUS_SYSTEM_CORRUPTED, "system corrupted");
    }
    
    /* save current stack pointer of the previous thread */
    current_thread->kmode_stack_ptr = (void *)(regs->esp - sizeof(struct isr_regs) - 4);

    /* switch to next thread */
    status = scheduler_set_current_thread(next_thread);
    if (!CHECK_SUCCESS(status)) return NULL;

    return next_thread->kmode_stack_ptr;
}

static void *pit_isr(int num, struct interrupt_frame *frame, struct isr_regs *regs, void *data)
{
    global_tick++;

    if (thread_is_preemption_enabled()) {
        return switch_thread(frame, regs);
    }

    return NULL;
}

static void init_pit(void)
{
    static const uint16_t pit_value = 1193182 / 100;
    
    io_out8(0x0043, 0x34);
    io_out8(0x0040, pit_value & 0xFF);
    io_out8(0x0040, (pit_value >> 8) & 0xFF);

    _pc_isr_unmask_interrupt(0x20);
}

void _pc_init_late(void)
{
    status_t status;

    LOG_DEBUG("initializing ISRs...\n");
    status = _pc_isr_init();
    if (!CHECK_SUCCESS(status)) {
        panic(status, "failed to initialize interrupt system");
    }

    _pc_pic_remap_int(0x20, 0x28);

    io_out8(0x0070, 0x8B);
    uint8_t temp = io_in8(0x0071);
    io_out8(0x0070, 0x8B);
    io_out8(0x0071, temp & ~0x70);

    LOG_DEBUG("testing whether invlpg available...\n");
    status = _pc_instruction_test(invlpg_test, 3, &_pc_invlpg_undefined);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "failed to test instruction invlpg");
    }

    LOG_DEBUG("testing whether rdtsc available...\n");
    status = _pc_instruction_test(rdtsc_test, 2, &_pc_rdtsc_undefined);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "failed to test instruction rdtsc");
    }

    _pc_isr_add_interrupt_handler(0x20, NULL, pit_isr, NULL);

    LOG_DEBUG("initializing PIT...\n");
    init_pit();
}
