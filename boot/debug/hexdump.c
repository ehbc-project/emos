#include <debug.h>

#include <stdio.h>
#include <string.h>

void hexdump(const void *data, long len, uint32_t offset)
{
    const uint8_t *addr = data;
    long count = 0;
    uint8_t buf[16];

    while (count < len) {
        printf("%08lX │ ", count + offset);

        memcpy(buf, addr, sizeof(buf));

        for (int i = 0; i < sizeof(buf) && count + i < len; i++) {
            printf("%02X ", buf[i]);

        }

        printf("│ ");

        for (int i = 0; i < sizeof(buf) && count + i < len; i++) {
            printf("%c", buf[i] >= 0x20 && buf[i] < 0x80 ? (char)buf[i] : '.');
        }
        
        printf("\n");

        addr += 16;
        count += 16;
    }
}
