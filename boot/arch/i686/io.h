#ifndef __I686_IO_H__
#define __I686_IO_H__

#include <stdint.h>

void _i686_out8(uint16_t port, uint8_t value);
void _i686_out16(uint16_t port, uint16_t value);
void _i686_out32(uint16_t port, uint32_t value);

uint8_t _i686_in8(uint16_t port);
uint16_t _i686_in16(uint16_t port);
uint32_t _i686_in32(uint16_t port);

#endif // __I686_IO_H__
