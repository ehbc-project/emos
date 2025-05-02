#include <stdint.h>
#include <stddef.h>

extern void _pc_bios_tty_output(uint8_t ch);
extern void _pc_bios_read_keyboard(uint8_t *scancode, char *ascii);

void main(void)
{
    
}
