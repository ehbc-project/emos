#ifndef __DRAW_H__
#define __DRAW_H__

#include <stdint.h>

#include "types.h"
#include "color.h"
#include "bitmap.h"

void draw_line(struct bitmap *target, const struct brush *b, struct point2 p0, struct point2 p1);
void draw_rect(struct bitmap *target, const struct brush *b, struct rect2 area, int fill);
void draw_bitmap(struct bitmap *target, struct rect2 area, const struct bitmap *src);

#endif // __DRAW_H__
