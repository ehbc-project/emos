#include <eboot/shell.h>

#include <stdio.h>

#include <eboot/status.h>
#include <eboot/mm.h>

static int mem_handler(struct shell_instance *inst, int argc, char **argv)
{
    status_t status;
    size_t total, used;

    status = mm_get_memory_usage(&total, &used);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "%s: unable to query memory usage\n", argv[0]);
        return 1;
    }

    printf("total %lu bytes, used %lu bytes\n", total, used);

    return 0;
}

static struct command mem_command = {
    .name = "mem",
    .handler = mem_handler,
    .help_message = "Print memory usage",
};

static void mem_command_init(void)
{
    shell_command_register(&mem_command);
}

REGISTER_SHELL_COMMAND(mem, mem_command_init)
