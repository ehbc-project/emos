#include <stdio.h>

#include <fs/fs.h>
#include <fs/driver.h>
#include <interface/char.h>

long ftell(FILE *stream)
{
    if (stream->dev || !stream->fs) {
        return 1;
    }

    return stream->file->fs->driver->tell(stream->file);
}
