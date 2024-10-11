#include <stdint.h>
#include <stddef.h>

extern void _pc_bios_tty_output(uint8_t ch);
extern void _pc_bios_read_keyboard(uint8_t *scancode, char *ascii);

void main(void)
{
    for (;;) {
        char ch;
        _pc_bios_read_keyboard(NULL, &ch);
        if (ch == '\r') {
            _pc_bios_tty_output('\n');
        }
        _pc_bios_tty_output(ch);
    }
}
