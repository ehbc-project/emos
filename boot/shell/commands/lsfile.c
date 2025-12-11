#include <eboot/shell.h>

#include <stdio.h>
#include <string.h>

#include <eboot/filesystem.h>

static int lsfile_handler(struct shell_instance *inst, int argc, char **argv)
{
    if (!inst->fs) {
        fprintf(stderr, "%s: filesystem not selected\n", argv[0]);
        return 1;
    }

    inst->working_dir->fs->driver->rewind_directory(inst->working_dir);

    struct fs_directory_entry direntry;
    while (!inst->working_dir->fs->driver->iter_directory(inst->working_dir, &direntry)) {
        if (strcmp(direntry.name, ".") == 0 || strcmp(direntry.name, "..") == 0) {
            continue;
        }

        printf("%8llu  %s\n", direntry.size, direntry.name);
    }

    return 0;
}

static struct command lsfile_command = {
    .name = "lsfile",
    .handler = lsfile_handler,
    .help_message = "List files of current directory",
};

static void lsfile_command_init(void)
{
    shell_command_register(&lsfile_command);
}

SHELL_COMMAND(lsfile, lsfile_command_init)
