#include <eboot/shell.h>

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <eboot/asm/bios/misc.h>

#include <eboot/status.h>

static int bootnext_handler(struct shell_instance *inst, int argc, char **argv)
{
    _pc_bios_bootnext();
    return 1;
}

static struct command bootnext_command = {
    .name = "bootnext",
    .handler = bootnext_handler,
    .help_message = "Boot from next device",
};

static void bootnext_command_init(void)
{
    shell_command_register(&bootnext_command);
}

REGISTER_SHELL_COMMAND(bootnext, bootnext_command_init)
