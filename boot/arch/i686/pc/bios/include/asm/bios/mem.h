#ifndef __I686_PC_BIOS_MEM_H__
#define __I686_PC_BIOS_MEM_H__

#include <stdint.h>

struct smap_entry {
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
};

long _pc_bios_query_address_map(uint32_t *cursor, struct smap_entry *buf, long buf_size);

#endif // __I686_PC_BIOS_MEM_H__
