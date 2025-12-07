#include <stdio.h>

#include <eboot/filesystem.h>
#include <eboot/interface/char.h>

int fseek(FILE *stream, long offset, int origin)
{
    switch (stream->type) {
        case 1:
            return stream->file.file->fs->driver->seek(stream->file.file, offset, origin);
        default:
            return 1;
    }
}
