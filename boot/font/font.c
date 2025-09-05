#include <stdint.h>

extern uint8_t build_boot_unifont_bfn[];

struct font_header {
  char signature[4];
  uint32_t max_codepoint;
  uint32_t glyph_offset_table_offset;
  uint8_t reserved[4];
} __attribute__((packed));

struct glyph_header {
  uint8_t is_full_width : 1;
  uint8_t : 7;
  uint8_t reserved[3];
} __attribute__((packed));

int font_is_glyph_full_width(uint32_t codepoint)
{
    struct font_header *header = (struct font_header*)build_boot_unifont_bfn;
    if (codepoint >= header->max_codepoint || !((uint32_t*)(build_boot_unifont_bfn + header->glyph_offset_table_offset))[codepoint]) return -1;
    struct glyph_header *glyph_header = (struct glyph_header*)((uint8_t*)build_boot_unifont_bfn + ((uint32_t*)(build_boot_unifont_bfn + header->glyph_offset_table_offset))[codepoint]);
    return glyph_header->is_full_width;
}

static void *memcpy(void *dest, const void *src, long n)
{
    uint8_t *d = (uint8_t*)dest;
    const uint8_t *s = (const uint8_t*)src;
    for (long i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

int font_get_glyph(uint32_t codepoint, uint8_t *buf, long size)
{
    struct font_header *header = (struct font_header*)build_boot_unifont_bfn;
    if (codepoint >= header->max_codepoint || !((uint32_t*)(build_boot_unifont_bfn + header->glyph_offset_table_offset))[codepoint]) return -1;
    struct glyph_header *glyph_header = (struct glyph_header*)((uint8_t*)build_boot_unifont_bfn + ((uint32_t*)(build_boot_unifont_bfn + header->glyph_offset_table_offset))[codepoint]);

    if (glyph_header->is_full_width && size < 32) return -1;
    if (!glyph_header->is_full_width && size < 16) return -1;

    memcpy(buf, ((uint8_t*)glyph_header) + sizeof(struct glyph_header), glyph_header->is_full_width ? 32 : 16);
    return 0;
}


