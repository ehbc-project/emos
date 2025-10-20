#include <stdio.h>

#include <interface/char.h>

size_t fwrite(const void *__restrict ptr, size_t size, size_t count, FILE *__restrict stream)
{
    size_t write_count = 0;

    while (write_count < count) {
        switch (stream->type) {
            case 2:
                if (stream->dev.charif->write(stream->dev.dev, ptr, size) != size) {
                    goto end;
                }
                break;
            default:
                return -1;
        }
        write_count++;
        ptr = (uint8_t *)ptr + size;
    }
end:

    return write_count;
}
