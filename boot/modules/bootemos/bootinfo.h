#ifndef __BOOTINFO_H__
#define __BOOTINFO_H__

#include <stdint.h>

#include <eboot/compiler.h>

#define BTV_CURRENT     0

#define BTF_BIGENDIAN   0x0001

struct bootinfo_table_header {
    uint16_t flags;
    uint16_t version;
    uint32_t entry_count;
    uint32_t entry_start;
    char strtab[];
} __packed;

struct bootinfo_entry_header {
    uint32_t type;
    uint32_t size;
} __packed;

#define BET_COMMAND_ARGS    0
#define BET_LOADER_INFO     1
#define BET_MEMORY_MAP      2
#define BET_SYSTEM_DISK     3
#define BET_ACPI_RSDP       4
#define BET_FRAMEBUFFER     5
#define BET_DEFAULT_FONT    6
#define BET_BOOT_GRAPHICS   7

struct bootinfo_entry_command_args {
    struct bootinfo_entry_header header;
    uint32_t arg_count;
    uint32_t arg_offsets[];
} __packed;

struct bootinfo_entry_loader_info {
    struct bootinfo_entry_header header;
    uint32_t additional_entry_count;
    uint32_t name_offset;
    uint32_t version_offset;
    uint32_t author_offset;
    uint32_t additional_entries[];
} __packed;

struct bootinfo_entry_memory_map {
    struct bootinfo_entry_header header;
    uint32_t entry_count;
    struct bootinfo_memory_map_entry {
        uint64_t base;
        uint64_t size;
        uint32_t type;
        uint32_t reserved;
    } __packed entries[];
} __packed;

struct bootinfo_entry_system_disk {
    struct bootinfo_entry_header header;
    uint32_t ident_crc32;
    uint32_t entry_count;
    struct bootinfo_system_disk_entry {
        uint64_t lba;
        uint32_t crc32;
    } __packed entries[];
} __packed;

struct bootinfo_entry_acpi_rsdp {
    struct bootinfo_entry_header header;
    char oemid[6];
    uint8_t revision;
    uint8_t reserved;
    uint32_t size;
    uint32_t rsdt_addr;
    uint64_t xsdt_addr;
} __packed;

#define BEFT_TEXT       0
#define BEFT_DIRECT     1

struct bootinfo_entry_framebuffer {
    struct bootinfo_entry_header header;
    uint64_t framebuffer_addr;
    uint32_t width;
    uint32_t pitch;
    uint32_t height;
    uint8_t bpp;
    uint8_t type;
    uint16_t reserved;
    union {
        struct bootinfo_framebuffer_direct_color_info {
            uint8_t red_pos;
            uint8_t red_size;
            uint8_t green_pos;
            uint8_t green_size;
            uint8_t blue_pos;
            uint8_t blue_size;
        } __packed direct;
    } __packed;
} __packed;

struct bootinfo_entry_default_font {
    struct bootinfo_entry_header header;
    uint8_t width;
    uint8_t height;
    uint8_t font_bpp;
    
} __packed;

#endif // __BOOTINFO_H__
