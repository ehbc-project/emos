#ifndef __MM_MM_H__
#define __MM_MM_H__

#include <stddef.h>

/**
 * @brief Allocate memory.
 * @param size The size of the memory to allocate.
 * @return A pointer to the allocated memory, or NULL if allocation fails.
 */
void *mm_allocate(size_t size);

/**
 * @brief Allocate memory and clear it to zero.
 * @param count The number of elements to allocate.
 * @param size The size of each element.
 * @return A pointer to the allocated and cleared memory, or NULL if allocation fails.
 */
void *mm_allocate_clear(unsigned long count, size_t size);

/**
 * @brief Free allocated memory.
 * @param ptr A pointer to the memory to free.
 */
void mm_free(void *ptr);

/**
 * @brief Reallocate memory.
 * @param ptr A pointer to the memory to reallocate.
 * @param size The new size of the memory.
 * @return A pointer to the reallocated memory, or NULL if reallocation fails.
 */
void *mm_reallocate(void *ptr, size_t size);

#endif // __MM_MM_H__
