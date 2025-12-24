#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <emos/compiler.h>
#include <emos/boot/bootinfo.h>

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

struct print_state {
    uint16_t *buffer;
    int col, row, width, height;
};

static int early_print_char(void *_state, char ch)
{
    struct print_state *state = _state;

    switch (ch) {
        case '\0':
            return 1;
        case '\n':
            state->row++;
        case '\r':
            state->col = 0;
            break;
        default:
            state->buffer[state->row * state->width + state->col] = ch | 0x0F00;
            state->col++;
            break;
    }

    if (state->col >= state->width) {
        state->row++;
        state->col = 0;
    }

    if (state->row >= state->height) {
        memmove(state->buffer, &state->buffer[state->width], state->width * (state->height - 1));
        state->row = state->height - 1;
    }

    return 0;
}

__attribute__((noreturn))
void main(struct bootinfo_table_header *btblhdr)
{
    struct bootinfo_entry_header *enthdr = (void *)((uintptr_t)btblhdr + btblhdr->entry_start);
    struct bootinfo_entry_command_args *caent;
    struct bootinfo_entry_framebuffer *fbent;
    struct print_state pstate;

    for (int i = 0; i < btblhdr->entry_count; i++) {
        if (enthdr->type == BET_COMMAND_ARGS) {
            caent = (void *)enthdr;
        } else if (enthdr->type == BET_FRAMEBUFFER) {
            fbent = (void *)enthdr;
        }

        enthdr = (void *)((uintptr_t)enthdr + enthdr->size);
    }

    pstate.buffer = (void *)(uintptr_t)fbent->framebuffer_addr;
    pstate.width = fbent->width;
    pstate.height = fbent->height;
    pstate.col = pstate.row = 0;

    for (int j = 0; j < caent->arg_count; j++) {
        cprintf(early_print_char, &pstate, "%s\n", &btblhdr->strtab[caent->arg_offsets[j]]);
    }

    cprintf(early_print_char, &pstate, "Hello, World! from Kernel\n");

    for (;;) {}
}
