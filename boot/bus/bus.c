#include <bus/bus.h>

#include <stddef.h>

static struct bus *bus_list_head = NULL;

int register_bus(struct bus *bus)
{
    if (!bus_list_head) {
        bus_list_head = bus;
        return 0;
    }

    struct bus *current = bus_list_head;
    while (current->next) {
        current = current->next;
    }
    current->next = bus;

    return 0;
}
