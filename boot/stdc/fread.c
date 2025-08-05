#include <stdio.h>

#include <interface/char.h>

size_t fread(void *ptr, size_t size, size_t count, FILE *stream)
{
    if (stream->device) {
        return stream->char_if->read(stream->device, ptr, size * count);
    }
    return 0;
}
