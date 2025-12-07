#include <stdio.h>

#include <eboot/filesystem.h>
#include <eboot/interface/char.h>

long ftell(FILE *stream)
{
    status_t status;
    off_t offset;

    switch (stream->type) {
        case 1:
            status = stream->file.file->fs->driver->tell(stream->file.file, &offset);
            if (!CHECK_SUCCESS(status)) return -1;
            return offset;
        default:
            return -1;
    }
}
