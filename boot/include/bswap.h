#ifndef __BSWAP_H__
#define __BSWAP_H__

#include <stdint.h>

#include <asm/processor.h>

#if defined(__has_builtin) && __has_builtin(__builtin_bswap16)
#   define _bswap16 __builtin_bswap16
#   define __HAVE_BUILTIN_BSWAP16
#else
uint16_t _bswap16(uint16_t val);
#endif

#if defined(__has_builtin) && __has_builtin(__builtin_bswap32)
#   define _bswap32 __builtin_bswap32
#   define __HAVE_BUILTIN_BSWAP32
#else
uint32_t _bswap32(uint32_t val);
#endif

#if defined(__has_builtin) && __has_builtin(__builtin_bswap64)
#   define _bswap64 __builtin_bswap64
#   define __HAVE_BUILTIN_BSWAP64
#else
uint64_t _bswap64(uint64_t val);
#endif

#if defined(__PROCESSOR_BIG_ENDIAN)
#   define CONV_BE_16(v) (v)
#   define CONV_BE_32(v) (v)
#   define CONV_BE_64(v) (v)

#   define CONV_LE_16(v) _bswap16(v)
#   define CONV_LE_32(v) _bswap32(v)
#   define CONV_LE_64(v) _bswap64(v)

#elif defined(__PROCESSOR_LITTLE_ENDIAN)
#   define CONV_BE_16(v) _bswap16(v)
#   define CONV_BE_32(v) _bswap32(v)
#   define CONV_BE_64(v) _bswap64(v)

#   define CONV_LE_16(v) (v)
#   define CONV_LE_32(v) (v)
#   define CONV_LE_64(v) (v)

#else
#   error Processor endianness is unknown

#endif

#endif // __BSWAP_H__