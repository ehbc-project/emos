#include <stdio.h>

#include <fs/fs.h>
#include <fs/driver.h>
#include <interface/char.h>

size_t fread(void *ptr, size_t size, size_t count, FILE *stream)
{
    size_t read_count = 0;

    while (read_count < count) {
        if (stream->dev && stream->charif->read(stream->dev, ptr, size) != size) {
            break;
        } else if (stream->fs && stream->file->fs->driver->read(stream->file, ptr, size) != size) {
            break;
        }
        read_count++;
        ptr = (uint8_t *)ptr + size;
    }

    return read_count;
}
