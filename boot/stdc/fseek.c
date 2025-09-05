#include <stdio.h>

#include <fs/fs.h>
#include <fs/driver.h>
#include <interface/char.h>

int fseek(FILE *stream, long offset, int origin)
{
    if (stream->dev || !stream->fs) {
        return 1;
    }

    return stream->file->fs->driver->seek(stream->file, offset, origin);
}
