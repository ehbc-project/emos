#ifndef __EBOOT_ASM_BIOS_MEM_H__
#define __EBOOT_ASM_BIOS_MEM_H__

#include <stdint.h>

#include <eboot/status.h>

struct smap_entry {
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
};

status_t _pc_bios_mem_query_map(uint32_t *cursor, struct smap_entry *buf, long buf_size);

#endif // __EBOOT_ASM_BIOS_MEM_H__
