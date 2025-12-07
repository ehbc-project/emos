#ifndef __EMOS_ASM_IOPORT_H__
#define __EMOS_ASM_IOPORT_H__

#include <stdint.h>

#include <emos/compiler.h>

static __always_inline void emos_ioport_write8(uint16_t port, uint8_t value)
{
    asm volatile ("outb %b0, %w1" : : "a"(value), "Nd"(port));
}

static __always_inline void emos_ioport_write16(uint16_t port, uint16_t value)
{
    asm volatile ("outw %w0, %w1" : : "a"(value), "Nd"(port));
}

static __always_inline void emos_ioport_write32(uint16_t port, uint32_t value)
{
    asm volatile ("outl %0, %w1" : : "a"(value), "Nd"(port));
}

static __always_inline void emos_ioport_writes8(uint16_t port, const uint8_t *data, unsigned long count)
{
    asm volatile ("rep outsb" : "+S"(data), "+c"(count) : "d"(port) : "memory");
}

static __always_inline void emos_ioport_writes16(uint16_t port, const uint16_t *data, unsigned long count)
{
    asm volatile ("rep outsw" : "+S"(data), "+c"(count) : "d"(port) : "memory");
}

static __always_inline void emos_ioport_writes32(uint16_t port, const uint32_t *data, unsigned long count)
{
    asm volatile ("rep outsl" : "+S"(data), "+c"(count) : "d"(port) : "memory");
}

static __always_inline uint8_t emos_ioport_read8(uint16_t port)
{
    uint8_t value;
    asm volatile ("inb %w1, %b0" : "=a"(value) : "Nd"(port));
    return value;
}

static __always_inline uint16_t emos_ioport_read16(uint16_t port)
{
    uint16_t value;
    asm volatile ("inw %w1, %w0" : "=a"(value) : "Nd"(port));
    return value;
}

static __always_inline uint32_t emos_ioport_read32(uint16_t port)
{
    uint32_t value;
    asm volatile ("inl %w1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static __always_inline void emos_ioport_reads8(uint16_t port, uint8_t *data, unsigned long count)
{
    asm volatile ("rep insb" : "+D"(data), "+c"(count) : "d"(port) : "memory");
}

static __always_inline void emos_ioport_reads16(uint16_t port, uint16_t *data, unsigned long count)
{
    asm volatile ("rep insw" : "+D"(data), "+c"(count) : "d"(port) : "memory");
}

static __always_inline void emos_ioport_reads32(uint16_t port, uint32_t *data, unsigned long count)
{
    asm volatile ("rep insl" : "+D"(data), "+c"(count) : "d"(port) : "memory");
}

#endif // __EMOS_ASM_IOPORT_H__
