#ifndef __I686_IO_H__
#define __I686_IO_H__

#include <stdint.h>

/**
 * @brief Write a byte to an I/O port.
 * @param port The I/O port address.
 * @param value The byte value to write.
 */
void _i686_out8(uint16_t port, uint8_t value);

/**
 * @brief Write a word to an I/O port.
 * @param port The I/O port address.
 * @param value The word value to write.
 */
void _i686_out16(uint16_t port, uint16_t value);

/**
 * @brief Write a double word to an I/O port.
 * @param port The I/O port address.
 * @param value The double word value to write.
 */
void _i686_out32(uint16_t port, uint32_t value);

/**
 * @brief Read a byte from an I/O port.
 * @param port The I/O port address.
 * @return The byte value read from the port.
 */
uint8_t _i686_in8(uint16_t port);

/**
 * @brief Read a word from an I/O port.
 * @param port The I/O port address.
 * @return The word value read from the port.
 */
uint16_t _i686_in16(uint16_t port);

/**
 * @brief Read a double word from an I/O port.
 * @param port The I/O port address.
 * @return The double word value read from the port.
 */
uint32_t _i686_in32(uint16_t port);

#endif // __I686_IO_H__
