#include <stdio.h>

#include <fs/fs.h>
#include <fs/driver.h>
#include <interface/char.h>

long ftell(FILE *stream)
{
    switch (stream->type) {
        case 1:
            return stream->file.file->fs->driver->tell(stream->file.file);
        default:
            return -1;
    }
}
