#include <eboot/shell.h>

#include <stdio.h>

#include <eboot/asm/isr.h>

static int lsint_handler(struct shell_instance *inst, int argc, char **argv)
{
    int null_start = -1;

    for (int i = 0; i < 256; i++) {
        struct isr_table_entry *entry = _pc_get_isr_table_entry(i);

        if (null_start >= 0 && (entry || i == 255)) {
            printf("Interrupt %d-%d: Unhandled\n", null_start, i - 1);
            null_start = -1;
        }

        if (!entry) {
            if (null_start < 0) {
                null_start = i;
            }
        } else {
            printf("Interrupt %d: \n", i);
            while (entry) {
                if (entry->interrupt_handler) {
                    if (entry->dev) {
                        printf("\ttype=int dev=%s handler=%p\n", entry->dev->name, (void *)entry->interrupt_handler);
                    } else {
                        printf("\ttype=int dev=none handler=%p\n", (void *)entry->interrupt_handler);
                    }
                } else {
                    printf("\ttype=trap handler=%p\n", (void *)entry->trap_handler);
                }

                entry = entry->next;
            }
        }
    }

    return 0;
}

static struct command lsint_command = {
    .name = "lsint",
    .handler = lsint_handler,
    .help_message = "List interrupt status",
};

static void lsint_command_init(void)
{
    shell_command_register(&lsint_command);
}

SHELL_COMMAND(lsint, lsint_command_init)
