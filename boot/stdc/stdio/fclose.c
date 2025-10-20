#include <stdio.h>

#include <mm/mm.h>
#include <fs/fs.h>
#include <fs/driver.h>

int fclose(FILE *stream)
{
    switch (stream->type) {
        case 1:
            stream->file.file->fs->driver->close(stream->file.file);
            break;
        default:
    }

    mm_free(stream);

    return 0;
}
