#ifndef __STRING_H__
#define __STRING_H__

#include <stddef.h>

/**
 * @brief Move a block of memory.
 * @param dest A pointer to the destination array where the content is to be copied.
 * @param src A pointer to the source of data to be copied.
 * @param len The number of bytes to copy.
 * @return A pointer to the destination.
 */
void *memmove(void *dest, const void *src, size_t len);  // not implemented yet

/**
 * @brief Compare two blocks of memory.
 * @param p1 A pointer to the first block of memory.
 * @param p2 A pointer to the second block of memory.
 * @param len The number of bytes to compare.
 * @return An integer less than, equal to, or greater than zero if the first len bytes of p1 are found, respectively, to be less than, to match, or be greater than the first len bytes of p2.
 */
int memcmp(const void *p1, const void *p2, size_t len);

/**
 * @brief Locate character in block of memory.
 * @param ptr A pointer to the block of memory.
 * @param value The value to search for.
 * @param len The number of bytes to search.
 * @return A pointer to the first occurrence of value in the block of memory, or NULL if not found.
 */
void *memchr(const void *ptr, int value, size_t len);  // not implemented yet

/**
 * @brief Fill a block of memory with a specified value.
 * @param dest A pointer to the block of memory to fill.
 * @param c The value to set.
 * @param count The number of bytes to set.
 * @return A pointer to the destination.
 */
void *memset(void *dest, int c, size_t count);

/**
 * @brief Fill a block of memory with a specified 16-bit value.
 * @param dest A pointer to the block of memory to fill.
 * @param c The 16-bit value to set.
 * @param count The number of 16-bit words to set.
 * @return A pointer to the destination.
 */
void *memset16(void *dest, int c, size_t count);

/**
 * @brief Copy a block of memory.
 * @param dest A pointer to the destination array where the content is to be copied.
 * @param src A pointer to the source of data to be copied.
 * @param len The number of bytes to copy.
 * @return A pointer to the destination.
 */
void *memcpy(void *dest, const void *src, size_t len);

/**
 * @brief Get error message string.
 * @param error The error number.
 * @return A pointer to the error message string.
 */
const char *strerror(int error);

/**
 * @brief Concatenate a string to another, with a maximum length.
 * @param dest A pointer to the destination string.
 * @param src A pointer to the source string.
 * @param maxlen The maximum number of characters to append from src.
 * @return A pointer to the destination string.
 */
char *strncat(char *dest, const char *src, size_t maxlen);

/**
 * @brief Concatenate a string to another.
 * @param dest A pointer to the destination string.
 * @param src A pointer to the source string.
 * @return A pointer to the destination string.
 */
char *strcat(char *dest, const char *src);

/**
 * @brief Copy a string, with a maximum length.
 * @param dest A pointer to the destination string.
 * @param src A pointer to the source string.
 * @param maxlen The maximum number of characters to copy from src.
 * @return A pointer to the destination string.
 */
char *strncpy(char *dest, const char *src, size_t maxlen);

/**
 * @brief Copy a string.
 * @param dest A pointer to the destination string.
 * @param src A pointer to the source string.
 * @return A pointer to the destination string.
 */
char *strcpy(char *dest, const char *src);

/**
 * @brief Compare two strings, with a maximum length.
 * @param p1 A pointer to the first string.
 * @param p2 A pointer to the second string.
 * @param maxlen The maximum number of characters to compare.
 * @return An integer less than, equal to, or greater than zero if the first maxlen bytes of p1 are found, respectively, to be less than, to match, or be greater than the first maxlen bytes of p2.
 */
int strncmp(const char *p1, const char *p2, size_t maxlen);

/**
 * @brief Compare two strings.
 * @param p1 A pointer to the first string.
 * @param p2 A pointer to the second string.
 * @return An integer less than, equal to, or greater than zero if the first string is found, respectively, to be less than, to match, or be greater than the second string.
 */
int strcmp(const char *p1, const char *p2);

/**
 * @brief Compare two strings, ignoring case, with a maximum length.
 * @param p1 A pointer to the first string.
 * @param p2 A pointer to the second string.
 * @param maxlen The maximum number of characters to compare.
 * @return An integer less than, equal to, or greater than zero if the first maxlen bytes of p1 are found, respectively, to be less than, to match, or be greater than the first maxlen bytes of p2, ignoring case.
 */
int strncasecmp(const char *p1, const char *p2, size_t maxlen);

/**
 * @brief Compare two strings, ignoring case.
 * @param p1 A pointer to the first string.
 * @param p2 A pointer to the second string.
 * @return An integer less than, equal to, or greater than zero if the first string is found, respectively, to be less than, to match, or be greater than the second string, ignoring case.
 */
int strcasecmp(const char *p1, const char *p2);

/**
 * @brief Locate first occurrence of character in string.
 * @param str A pointer to the string.
 * @param ch The character to search for.
 * @return A pointer to the first occurrence of ch in str, or NULL if not found.
 */
char *strchr(const char *str, int ch);

/**
 * @brief Locate last occurrence of character in string.
 * @param str A pointer to the string.
 * @param ch The character to search for.
 * @return A pointer to the last occurrence of ch in str, or NULL if not found.
 */
char *strrchr(const char *str, int ch);

/**
 * @brief Locate substring.
 * @param str A pointer to the string to search.
 * @param substr A pointer to the substring to search for.
 * @return A pointer to the first occurrence of substr in str, or NULL if not found.
 */
char *strstr(const char *str, const char *substr);  // not implemented yet

/**
 * @brief Calculate the length of the initial segment of a string that consists of characters not in a specified set.
 * @param p1 A pointer to the string to search.
 * @param p2 A pointer to the string containing the characters to search for.
 * @return The length of the initial segment of p1 that does not contain any character from p2.
 */
size_t strcspn(const char *p1, const char *p2);  // not implemented yet

/**
 * @brief Calculate the length of the initial segment of a string that consists of characters in a specified set.
 * @param p1 A pointer to the string to search.
 * @param p2 A pointer to the string containing the characters to search for.
 * @return The length of the initial segment of p1 that contains only characters from p2.
 */
size_t strspn(const char *p1, const char *p2);  // not implemented yet

/**
 * @brief Locate first occurrence of any character from a set in a string.
 * @param p1 A pointer to the string to search.
 * @param p2 A pointer to the string containing the characters to search for.
 * @return A pointer to the first occurrence in p1 of any character from p2, or NULL if not found.
 */
char *strpbrk(const char *p1, const char *p2);  // not implemented yet

/**
 * @brief Tokenize a string.
 * @param str A pointer to the string to tokenize. On first call, this is the string to tokenize. On subsequent calls, this should be NULL.
 * @param delim A pointer to the string containing the delimiters.
 * @return A pointer to the next token, or NULL if no more tokens.
 */
char *strtok(char *str, const char *delim);

/**
 * @brief Calculate the length of a string, with a maximum length.
 * @param str A pointer to the string.
 * @param maxlen The maximum number of characters to check.
 * @return The length of the string, or maxlen if the null terminator is not found within maxlen characters.
 */
size_t strnlen(const char *str, size_t maxlen);

/**
 * @brief Calculate the length of a string.
 * @param str A pointer to the string.
 * @return The length of the string.
 */
size_t strlen(const char *str);

#endif // __STRING_H__