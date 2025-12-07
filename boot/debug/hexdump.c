#include <eboot/debug.h>

#include <stdio.h>
#include <string.h>

void hexdump(const void *data, long len, uint32_t offset)
{
    const uint8_t *addr = data;
    long count = 0;
    uint8_t buf[16];

    while (count < len) {
        fprintf(stddbg, "%08lX │ ", count + offset);

        memcpy(buf, addr, sizeof(buf));

        for (int i = 0; i < sizeof(buf) && count + i < len; i++) {
            fprintf(stddbg, "%02X ", buf[i]);

        }

        fprintf(stddbg, "│ ");

        for (int i = 0; i < sizeof(buf) && count + i < len; i++) {
            fprintf(stddbg, "%c", buf[i] >= 0x20 && buf[i] < 0x80 ? (char)buf[i] : '.');
        }
        
        fprintf(stddbg, "\n");

        addr += 16;
        count += 16;
    }
}
