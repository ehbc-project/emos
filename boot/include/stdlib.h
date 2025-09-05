#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <stddef.h>

#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1

#define RAND_MAX        0x7FFF

/**
 * @brief Terminate the calling process normally.
 * @param status The exit status.
 */
__attribute__((noreturn))
void exit(int status);

/**
 * @brief Terminate the calling process normally, without calling atexit functions.
 * @param status The exit status.
 */
__attribute__((noreturn))
void _Exit(int status);

/**
 * @brief Terminate the calling process normally, without calling atexit functions.
 * @param status The exit status.
 */
__attribute__((noreturn))
void quick_exit(int status);

/**
 * @brief Register a function to be called at normal program termination.
 * @param func The function to register.
 * @return 0 on success, non-zero on failure.
 */
int atexit(void (*func)(void));

/**
 * @brief Register a function to be called at quick program termination.
 * @param func The function to register.
 * @return 0 on success, non-zero on failure.
 */
int at_quick_exit(void (*func)(void));

/**
 * @brief Convert string to double.
 * @param str The string to convert.
 * @return The converted double value.
 */
double atof(const char *str);

/**
 * @brief Convert string to integer.
 * @param str The string to convert.
 * @return The converted integer value.
 */
int atoi(const char *str);

/**
 * @brief Convert string to long integer.
 * @param str The string to convert.
 * @return The converted long integer value.
 */
long atol(const char *str);

/**
 * @brief Convert string to long long integer.
 * @param str The string to convert.
 * @return The converted long long integer value.
 */
long long atoll(const char *str);

/**
 * @brief Convert string to float.
 * @param str The string to convert.
 * @param endptr A pointer to a pointer to the character after the last character used in the conversion.
 * @return The converted float value.
 */
float strtof(const char *str, char **endptr);

/**
 * @brief Convert string to double.
 * @param str The string to convert.
 * @param endptr A pointer to a pointer to the character after the last character used in the conversion.
 * @return The converted double value.
 */
double strtod(const char *str, char **endptr);

/**
 * @brief Convert string to long double.
 * @param str The string to convert.
 * @param endptr A pointer to a pointer to the character after the last character used in the conversion.
 * @return The converted long double value.
 */
long double strtold(const char *str, char **endptr);

/**
 * @brief Convert string to long integer with a specified base.
 * @param str The string to convert.
 * @param endptr A pointer to a pointer to the character after the last character used in the conversion.
 * @param base The base of the number.
 * @return The converted long integer value.
 */
long strtol(const char *str, char **endptr, int base);

/**
 * @brief Convert string to long long integer with a specified base.
 * @param str The string to convert.
 * @param endptr A pointer to a pointer to the character after the last character used in the conversion.
 * @param base The base of the number.
 * @return The converted long long integer value.
 */
long long strtoll(const char *str, char **endptr, int base);

/**
 * @brief Convert string to unsigned long integer with a specified base.
 * @param str The string to convert.
 * @param endptr A pointer to a pointer to the character after the last character used in the conversion.
 * @param base The base of the number.
 * @return The converted unsigned long integer value.
 */
unsigned long strtoul(const char *str, char **endptr, int base);

/**
 * @brief Convert string to unsigned long long integer with a specified base.
 * @param str The string to convert.
 * @param endptr A pointer to a pointer to the character after the last character used in the conversion.
 * @param base The base of the number.
 * @return The converted unsigned long long integer value.
 */
unsigned long long strtoull(const char *str, char **endptr, int base);

/**
 * @brief Sort an array using the quicksort algorithm.
 * @param base A pointer to the first element of the array.
 * @param num The number of elements in the array.
 * @param size The size of each element in bytes.
 * @param cmp A pointer to the comparison function.
 */
void qsort(void *base, size_t num, size_t size, int (*cmp)(const void*, const void*));

/**
 * @brief Perform a binary search on a sorted array.
 * @param key A pointer to the value to search for.
 * @param base A pointer to the first element of the array.
 * @param num The number of elements in the array.
 * @param size The size of each element in bytes.
 * @param cmp A pointer to the comparison function.
 * @return A pointer to the matching element, or NULL if not found.
 */
void *bsearch(const void *key, const void *base, size_t num, size_t size, int (*cmp)(const void*, const void*));

typedef struct {
    int quot;
    int rem;
} div_t;

typedef struct {
    long quot;
    long rem;
} ldiv_t;

typedef struct {
    long long quot;
    long long rem;
} lldiv_t;

/**
 * @brief Compute the absolute value of an integer.
 * @param n The integer value.
 * @return The absolute value.
 */
int abs(int n);

/**
 * @brief Compute the quotient and remainder of integer division.
 * @param numer The numerator.
 * @param denom The denominator.
 * @return A div_t structure containing the quotient and remainder.
 */
div_t div(int numer, int denom);

/**
 * @brief Compute the absolute value of a long integer.
 * @param n The long integer value.
 * @return The absolute value.
 */
long labs(long n);

/**
 * @brief Compute the quotient and remainder of long integer division.
 * @param numer The numerator.
 * @param denom The denominator.
 * @return An ldiv_t structure containing the quotient and remainder.
 */
ldiv_t ldiv(long numer, long denom);

/**
 * @brief Compute the absolute value of a long long integer.
 * @param n The long long integer value.
 * @return The absolute value.
 */
long long llabs(long long n);

/**
 * @brief Compute the quotient and remainder of long long integer division.
 * @param numer The numerator.
 * @param denom The denominator.
 * @return An lldiv_t structure containing the quotient and remainder.
 */
lldiv_t lldiv(long long numer, long long denom);


/**
 * @brief Generate a pseudo-random integer.
 * @return A pseudo-random integer in the range [0, RAND_MAX].
 */
int rand(void);

/**
 * @brief Seed the pseudo-random number generator.
 * @param seed The seed value.
 */
void srand(unsigned int seed);

/**
 * @brief Allocate memory.
 * @param size The size of the memory to allocate.
 * @return A pointer to the allocated memory, or NULL if allocation fails.
 */
void *malloc(size_t size);

/**
 * @brief Allocate memory for an array and initialize to zero.
 * @param num The number of elements.
 * @param size The size of each element.
 * @return A pointer to the allocated memory, or NULL if allocation fails.
 */
void *calloc(size_t num, size_t size);

/**
 * @brief Reallocate memory.
 * @param ptr A pointer to the memory to reallocate.
 * @param size The new size of the memory.
 * @return A pointer to the reallocated memory, or NULL if reallocation fails.
 */
void *realloc(void *ptr, size_t size);

/**
 * @brief Free allocated memory.
 * @param ptr A pointer to the memory to free.
 */
void free(void *ptr);

#endif // __STDLIB_H__