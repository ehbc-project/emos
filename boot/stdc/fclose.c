#include <stdio.h>

#include <mm/mm.h>
#include <fs/fs.h>
#include <fs/driver.h>

int fclose(FILE *stream)
{
    if (stream->fs) {
        stream->file->fs->driver->close(stream->file);
    }

    mm_free(stream);

    return 0;
}
