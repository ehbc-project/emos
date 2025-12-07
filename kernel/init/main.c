#include <stdint.h>

extern const uint8_t image_map[307200 * 3];

__attribute__((noreturn))
void main(char *args)
{
    uint32_t *fb = (uint32_t *)0xFD000000;
    for (int i = 0; i < 640 * 480; i++) {
        fb[i] = image_map[i * 3] | (image_map[i * 3 + 1] << 8) | (image_map[i * 3 + 2] << 16);
    }

    for (int i = 0; args[i]; i++) {
        asm volatile ("outb %b0, $0xE9" : : "a"(args[i]));
    }


    for (;;) {}
}
