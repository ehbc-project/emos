#include <stdio.h>

#include <fs/fs.h>
#include <fs/driver.h>
#include <interface/char.h>

size_t fread(void *__restrict ptr, size_t size, size_t count, FILE *__restrict stream)
{
    size_t read_count = 0;

    while (read_count < count) {
        switch (stream->type) {
            case 1:
                if (stream->file.file->fs->driver->read(stream->file.file, ptr, size) != size) {
                    goto end;
                }
                break;
            case 2:
                if (stream->dev.charif->read(stream->dev.dev, ptr, size) != size) {
                    goto end;
                }
                break;
            default:
        }
        read_count++;
        ptr = (uint8_t *)ptr + size;
    }
end:

    return read_count;
}
