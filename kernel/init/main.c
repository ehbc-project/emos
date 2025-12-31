#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <emos/asm/gdt.h>
#include <emos/asm/pc_gdt.h>
#include <emos/asm/io.h>
#include <emos/asm/page.h>
#include <emos/asm/interrupt.h>
#include <emos/asm/instruction.h>

#include <emos/compiler.h>
#include <emos/boot/bootinfo.h>
#include <emos/mm.h>
#include <emos/thread.h>
#include <emos/status.h>
#include <emos/macros.h>
#include <emos/panic.h>
#include <emos/log.h>

#define MODULE_NAME "main"

// extern const uint8_t image_map[307200 * 3];
// extern const uint8_t pbar_map[20000 * 4];
// 
// uint8_t *fb = (void *)(uintptr_t)fbent->framebuffer_addr;
// int pitch = fbent->pitch;
// 
// for (int i = 0; i < 480; i++) {
//     for (int j = 0; j < 640; j++) {
//         fb[i * pitch + j * 3] = image_map[(i * 640 + j) * 3];
//         fb[i * pitch + j * 3 + 1] = image_map[(i * 640 + j) * 3 + 1];
//         fb[i * pitch + j * 3 + 2] = image_map[(i * 640 + j) * 3 + 2];
//     }
// }
// 
// int center_x = 320;
// int center_y = 375;
// 
// for (int f = 0;; f = (f + 1) % 8) {
//     for (int i = center_y - 25; i < center_y + 25; i++) {
//         for (int j = center_x - 25; j < center_x + 25; j++) {
//             int alpha = pbar_map[((i - center_y + 25) * 400 + (j - center_x + 25 + f * 50)) * 4 + 3];
//             int r = pbar_map[((i - center_y + 25) * 400 + (j - center_x + 25 + f * 50)) * 4 + 0];
//             int g = pbar_map[((i - center_y + 25) * 400 + (j - center_x + 25 + f * 50)) * 4 + 1];
//             int b = pbar_map[((i - center_y + 25) * 400 + (j - center_x + 25 + f * 50)) * 4 + 2];
//             int bg_r = image_map[(i * 640 + j) * 3];
//             int bg_g = image_map[(i * 640 + j) * 3 + 1];
//             int bg_b = image_map[(i * 640 + j) * 3 + 2];
// 
//             if (!alpha) {
//                 fb[i * pitch + j * 3] = bg_r;
//                 fb[i * pitch + j * 3 + 1] = bg_g;
//                 fb[i * pitch + j * 3 + 2] = bg_b;
//             } else if (alpha == 255) {
//                 fb[i * pitch + j * 3] = r;
//                 fb[i * pitch + j * 3 + 1] = g;
//                 fb[i * pitch + j * 3 + 2] = b;
//             } else {
//                 fb[i * pitch + j * 3] = bg_r * (255 - alpha) / 255 + r * alpha / 255;
//                 fb[i * pitch + j * 3 + 1] = bg_g * (255 - alpha) / 255 + g * alpha / 255;
//                 fb[i * pitch + j * 3 + 2] = bg_b * (255 - alpha) / 255 + b * alpha / 255;
//             }
//         }
//     }
// 
//     for (volatile int i = 0; i < 1024 * 1024 * 16; i++) {}
// }

extern struct bootinfo_table_header *_pc_bootinfo_table;

struct print_state {
    uint16_t *framebuffer;
    int width, pitch, height;
    int cursor_col, cursor_row;
};

static int early_print_char(void *_state, char ch)
{
    struct print_state *state = _state;
    uint16_t *framebuffer;
    int width, height, pitch, line_diff;

    framebuffer = state->framebuffer;
    width = state->width;
    height = state->height;
    pitch = state->pitch;

    switch (ch) {
        case '\0':
            return 1;
        case '\n':
            state->cursor_row++;
        case '\r':
            state->cursor_col = 0;
            break;
        case '\t':
            state->cursor_col = (state->cursor_col + 8) & ~7;
            break;
        case '\b':
            state->cursor_col--;
            break;
        default:
            framebuffer[state->cursor_row * width + state->cursor_col] = ch | 0x0700;
            state->cursor_col++;
            break;
    }

    if (state->cursor_col >= width) {
        state->cursor_row += state->cursor_col / width;
        state->cursor_col %= width;
    }

    if (state->cursor_row >= height) {
        line_diff = state->cursor_row - height + 1;
        for (int i = 0; i < height - line_diff; i++) {
            memcpy(&framebuffer[i * width], &framebuffer[(i + line_diff) * width], pitch);
        }
        memset(&framebuffer[(height - line_diff) * width], 0, pitch * line_diff);
        state->cursor_row = height - 1;
    }

    return 0;
}

static uint16_t *fb;

static void thread1_main(struct thread *th)
{
    uint32_t index = 0;

    for (;;) {
        index = (index + 1) % (80 * 25);

        fb[index] = '\\' | 0x0700;
    }
}

static void thread2_main(struct thread *th)
{
    uint32_t index = 0;

    for (;;) {
        index = (index + 1) % (80 * 25);

        fb[index] = '|' | 0x0700;
    }
}

static void thread3_main(struct thread *th)
{
    uint32_t index = 0;

    for (;;) {
        index = (index + 1) % (80 * 25);

        fb[index] = '/' | 0x0700;
    }
}

__attribute__((noreturn))
void main(void)
{
    status_t status;
    struct bootinfo_entry_header *enthdr = NULL;
    struct bootinfo_entry_command_args *caent = NULL;
    struct bootinfo_entry_loader_info *lient = NULL;
    struct bootinfo_entry_memory_map *mment = NULL;
    struct bootinfo_entry_system_disk *sdent = NULL;
    struct bootinfo_entry_acpi_rsdp *arent = NULL;
    struct bootinfo_entry_framebuffer *fbent = NULL;
    struct bootinfo_entry_default_font *dfent = NULL;
    struct bootinfo_entry_boot_graphics *bgent = NULL;
    struct bootinfo_entry_unavailable_frames *ufent = NULL;
    struct bootinfo_entry_pagetable_vpn *pvent = NULL;
    vpn_t earlyfb_vpn;
    struct print_state pstate;

    enthdr = (void *)((uintptr_t)_pc_bootinfo_table + _pc_bootinfo_table->header_size);
    for (int i = 0; i < _pc_bootinfo_table->entry_count; i++) {
        switch (enthdr->type) {
            case BET_COMMAND_ARGS:
                caent = (void *)((uintptr_t)enthdr + enthdr->header_size);
                break;
            case BET_LOADER_INFO:
                lient = (void *)((uintptr_t)enthdr + enthdr->header_size);
                break;
            case BET_MEMORY_MAP:
                mment = (void *)((uintptr_t)enthdr + enthdr->header_size);
                break;
            case BET_SYSTEM_DISK:
                sdent = (void *)((uintptr_t)enthdr + enthdr->header_size);
                break;
            case BET_ACPI_RSDP:
                arent = (void *)((uintptr_t)enthdr + enthdr->header_size);
                break;
            case BET_FRAMEBUFFER:
                fbent = (void *)((uintptr_t)enthdr + enthdr->header_size);
                break;
            case BET_DEFAULT_FONT:
                dfent = (void *)((uintptr_t)enthdr + enthdr->header_size);
                break;
            case BET_BOOT_GRAPHICS:
                bgent = (void *)((uintptr_t)enthdr + enthdr->header_size);
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

    /* hang if there's no framebuffer or not in text mode */
    if (!fbent || fbent->type != BEFT_TEXT) {
        for (;;) {}
    }

    status = mm_vma_allocate_page(ALIGN_DIV(fbent->pitch * fbent->height, PAGE_SIZE), &earlyfb_vpn, VAF_KERNEL);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "cannot allocate pages for early framebuffer");
    }

    status = mm_map(fbent->framebuffer_addr / PAGE_SIZE, earlyfb_vpn, ALIGN_DIV(fbent->pitch * fbent->height, PAGE_SIZE), PMF_WTCACHE);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "cannot map early framebuffer");
    }

    pstate.framebuffer = (uint16_t *)(earlyfb_vpn * PAGE_SIZE);
    pstate.width = fbent->width;
    pstate.height = fbent->height;
    pstate.pitch = fbent->pitch;
    pstate.cursor_col = pstate.cursor_row = 0;

    fb = pstate.framebuffer;

    LOG_DEBUG("reinitializing early logger...\n");
    log_early_init(early_print_char, &pstate);

    /* print entries */
    if (caent) {
        LOG_DEBUG("command args entry:\n");
        for (int j = 0; j < caent->arg_count; j++) {
            LOG_DEBUG("\t%s\n", &_pc_bootinfo_table->strtab[caent->arg_offsets[j]]);
        }
    }
    
    if (lient) {
        LOG_DEBUG("loader info entry:\n");
        LOG_DEBUG("\tname: %s\n", &_pc_bootinfo_table->strtab[lient->name_offset]);
        LOG_DEBUG("\tversion: %s\n", &_pc_bootinfo_table->strtab[lient->version_offset]);
        LOG_DEBUG("\tauthor: %s\n", &_pc_bootinfo_table->strtab[lient->author_offset]);

        if (lient->additional_entry_count > 0) {
            LOG_DEBUG("\tadditional entries:\n");
        }
        for (int j = 0; j < lient->additional_entry_count; j++) {
            LOG_DEBUG("\t\t%s\n", &_pc_bootinfo_table->strtab[lient->additional_entries[j]]);
        }
    }

    if (mment) {
        LOG_DEBUG("memory map entry:\n");
        LOG_DEBUG("\tbase             size             type\n");
        for (int j = 0; j < mment->entry_count; j++) {
            LOG_DEBUG("\t%016llX %016llX %08lX\n", mment->entries[j].base, mment->entries[j].size, mment->entries[j].type);
        }
    }
    
    if (sdent) {
        LOG_DEBUG("system disk entry:\n");
        LOG_DEBUG("\tident_crc32: %08lX\n", sdent->ident_crc32);
        LOG_DEBUG("\tlba              crc32\n");
        for (int j = 0; j < sdent->entry_count; j++) {
            LOG_DEBUG("\t%016llX %08lX\n", sdent->entries[j].lba, sdent->entries[j].crc32);
        }
    }
    
    if (arent) {
        LOG_DEBUG("acpi rsdp entry:\n");
        LOG_DEBUG("\toemid: %.6s\n", arent->oemid);
        LOG_DEBUG("\trevision: %02X\n", arent->revision);
        LOG_DEBUG("\tsize: %08lX\n", arent->size);
        LOG_DEBUG("\trsdt: %08lX\n", arent->rsdt_addr);
        LOG_DEBUG("\txsdt: %016llX\n", arent->xsdt_addr);
    }
    
    if (dfent) {
        LOG_DEBUG("default font entry:\n");
    }
    
    if (bgent) {
        LOG_DEBUG("boot graphics entry:\n");
    }

    if (ufent) {
        LOG_DEBUG("unavailable frames entry:\n");
        LOG_DEBUG("\tpfn              type\n");
        for (int j = 0; j < ufent->entry_count; j++) {
            LOG_DEBUG("\t%016llX %01X\n", (uint64_t)ufent->entries[j].pfn, ufent->entries[j].type);
        }
    }

    if (pvent) {
        LOG_DEBUG("pagetable vpn entry:\n");
        LOG_DEBUG("\t%016llX\n", pvent->vpn);
    }

    LOG_DEBUG("Starting main...\n");

    size_t total_frames, free_frames;
    size_t kernel_total_pages, kernel_free_pages, user_total_pages, user_free_pages;
    
    mm_pma_get_available_frame_count(&total_frames);
    mm_pma_get_free_frame_count(&free_frames);
    mm_vma_get_available_kernel_page_count(&kernel_total_pages);
    mm_vma_get_free_kernel_page_count(&kernel_free_pages);
    mm_vma_get_available_user_page_count(&user_total_pages);
    mm_vma_get_free_user_page_count(&user_free_pages);
    
    LOG_DEBUG("frame           kernel page     user page\n");
    LOG_DEBUG("  avail    free   avail    free   avail    free\n");
    LOG_DEBUG("%7lu %7lu %7lu %7lu %7lu %7lu\n", total_frames, free_frames, kernel_total_pages, kernel_free_pages, user_total_pages, user_free_pages);

    struct thread *kthread;
    struct thread *thread1;
    struct thread *thread2;
    struct thread *thread3;

    thread_create(NULL, 0, &kthread);
    
    thread_init();
    thread_start_preemption();

    thread_create(thread1_main, 0x10000, &thread1);
    thread_create(thread2_main, 0x10000, &thread2);
    thread_create(thread3_main, 0x10000, &thread3);

    io_out8(0x007A, 0x00);
    io_out8(0x007B, 0x00);

    uint32_t index = 0;

    for (;;) {
        index = (index + 1) % (80 * 25);

        fb[index] = '-' | 0x0700;
    }
}
