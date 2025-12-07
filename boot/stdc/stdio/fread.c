#include <stdio.h>

#include <eboot/filesystem.h>
#include <eboot/interface/char.h>

size_t fread(void *__restrict ptr, size_t size, size_t count, FILE *__restrict stream)
{
    status_t status;
    size_t read_count = 0;

    while (read_count < count) {
        switch (stream->type) {
            case 1:
                status = stream->file.file->fs->driver->read(stream->file.file, ptr, size, NULL);
                if (!CHECK_SUCCESS(status)) goto end;
                break;
            case 2:
                status = stream->dev.charif->read(stream->dev.dev, ptr, size, NULL);
                if (!CHECK_SUCCESS(status)) goto end;
                break;
            case 3:
                return stream->cookie.io_funcs.read(stream->cookie.cookie, ptr, size);
            default:
                return -1;
        }
        read_count++;
        ptr = (uint8_t *)ptr + size;
    }
end:

    return read_count;
}
