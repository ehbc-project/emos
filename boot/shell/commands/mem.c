#include <eboot/shell.h>

#include <stdio.h>

#include <eboot/asm/bios/mem.h>

#include <eboot/status.h>
#include <eboot/mm.h>

static int mem_handler(struct shell_instance *inst, int argc, char **argv)
{
    status_t status;
    size_t total, used;
    uint32_t cursor = 0;
    struct smap_entry entry;

    static const char *region_type[] ={
        "", "Free", "Reserved", "ACPI Reclaimable", "ACPI NVS", "Bad",
    };

    printf("Memory Map:\n");
    printf("Base                Size                Type\n");
    do {
        _pc_bios_mem_query_map(&cursor, &entry, sizeof(entry));
        printf("%08lX%08lX    %08lX%08lX    %s\n", entry.base_addr_high, entry.base_addr_low, entry.length_high, entry.length_low, region_type[entry.type]);
    } while (cursor);


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
