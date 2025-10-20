#ifndef __I686_IO_H__
#define __I686_IO_H__

#include <stdint.h>

#include <compiler.h>

__always_inline void io_out8(uint16_t port, uint8_t value)
{
    asm volatile ("outb %b0, %w1" : : "a"(value), "Nd"(port));
}

__always_inline void io_out16(uint16_t port, uint16_t value)
{
    asm volatile ("outw %w0, %w1" : : "a"(value), "Nd"(port));
}

__always_inline void io_out32(uint16_t port, uint32_t value)
{
    asm volatile ("outl %0, %w1" : : "a"(value), "Nd"(port));
}

__always_inline void io_outs8(uint16_t port, const uint8_t *data, unsigned long count)
{
    asm volatile ("rep outsb" : "+S"(data), "+c"(count) : "d"(port) : "memory");
}

__always_inline void io_outs16(uint16_t port, const uint16_t *data, unsigned long count)
{
    asm volatile ("rep outsw" : "+S"(data), "+c"(count) : "d"(port) : "memory");
}

__always_inline void io_outs32(uint16_t port, const uint32_t *data, unsigned long count)
{
    asm volatile ("rep outsl" : "+S"(data), "+c"(count) : "d"(port) : "memory");
}

__always_inline uint8_t io_in8(uint16_t port)
{
    uint8_t value;
    asm volatile ("inb %w1, %b0" : "=a"(value) : "Nd"(port));
    return value;
}

__always_inline uint16_t io_in16(uint16_t port)
{
    uint16_t value;
    asm volatile ("inw %w1, %w0" : "=a"(value) : "Nd"(port));
    return value;
}

__always_inline uint32_t io_in32(uint16_t port)
{
    uint32_t value;
    asm volatile ("inl %w1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

__always_inline void io_ins8(uint16_t port, uint8_t *data, unsigned long count)
{
    asm volatile ("rep insb" : "+D"(data), "+c"(count) : "d"(port) : "memory");
}

__always_inline void io_ins16(uint16_t port, uint16_t *data, unsigned long count)
{
    asm volatile ("rep insw" : "+D"(data), "+c"(count) : "d"(port) : "memory");
}

__always_inline void io_ins32(uint16_t port, uint32_t *data, unsigned long count)
{
    asm volatile ("rep insl" : "+D"(data), "+c"(count) : "d"(port) : "memory");
}

#endif // __I686_IO_H__
