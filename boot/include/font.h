#ifndef __FONT_H__
#define __FONT_H__

#include <wchar.h>

int font_use(const char *path);

int font_get_glyph_dimension(wchar_t codepoint, int *width, int *height);
int font_get_glyph_data(wchar_t codepoint, uint8_t *buf, long size);

#endif // __FONT_H__
