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

/**
 * @brief Query the system address map using BIOS interrupt 0x15, E820h.
 * @param cursor A pointer to a cursor value for iteration. Initialize to 0 for the first call.
 * @param buf A pointer to a buffer to store the SMAP entry.
 * @param buf_size The size of the buffer.
 * @return The number of bytes written to the buffer, or 0 if no more entries.
 */
long _pc_bios_query_address_map(uint32_t *cursor, struct smap_entry *buf, long buf_size);

#endif // __I686_PC_BIOS_MEM_H__
