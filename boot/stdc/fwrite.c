#include <stdio.h>

#include <interface/char.h>

size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream)
{
    if (stream->dev) {
        return stream->charif->write(stream->dev, ptr, size * count);
    }
    return 0;
}
