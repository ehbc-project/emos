#include <stdint.h>

__attribute__((noreturn))
void main(char *args)
{
    uint32_t *fb = (uint32_t *)0xFD000000;
    for (int i = 0; i < 640 * 480 * 3 / 4; i++) {
        fb[i] = 0xFFFFFFFF;
    }

    for (int i = 0; args[i]; i++) {
        asm volatile ("outb %b0, $0xE9" : : "a"(args[i]));
    }

    for (;;) {}
}
