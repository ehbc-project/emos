#ifndef __CTYPE_H__
#define __CTYPE_H__

/**
 * @brief Check if character is alphanumeric.
 * @param c The character to check.
 * @return Non-zero if the character is alphanumeric, 0 otherwise.
 */
int isalnum(int c);

/**
 * @brief Check if character is alphabetic.
 * @param c The character to check.
 * @return Non-zero if the character is alphabetic, 0 otherwise.
 */
int isalpha(int c);

/**
 * @brief Check if character is a control character.
 * @param c The character to check.
 * @return Non-zero if the character is a control character, 0 otherwise.
 */
int iscntrl(int c);

/**
 * @brief Check if character is a digit.
 * @param c The character to check.
 * @return Non-zero if the character is a digit, 0 otherwise.
 */
int isdigit(int c);

/**
 * @brief Check if character is a graphic character.
 * @param c The character to check.
 * @return Non-zero if the character is a graphic character, 0 otherwise.
 */
int isgraph(int c);

/**
 * @brief Check if character is lowercase.
 * @param c The character to check.
 * @return Non-zero if the character is lowercase, 0 otherwise.
 */
int islower(int c);

/**
 * @brief Check if character is printable.
 * @param c The character to check.
 * @return Non-zero if the character is printable, 0 otherwise.
 */
int isprint(int c);

/**
 * @brief Check if character is a punctuation character.
 * @param c The character to check.
 * @return Non-zero if the character is a punctuation character, 0 otherwise.
 */
int ispunct(int c);

/**
 * @brief Check if character is a whitespace character.
 * @param c The character to check.
 * @return Non-zero if the character is a whitespace character, 0 otherwise.
 */
int isspace(int c);

/**
 * @brief Check if character is uppercase.
 * @param c The character to check.
 * @return Non-zero if the character is uppercase, 0 otherwise.
 */
int isupper(int c);

/**
 * @brief Check if character is a hexadecimal digit.
 * @param c The character to check.
 * @return Non-zero if the character is a hexadecimal digit, 0 otherwise.
 */
int isxdigit(int c);

/**
 * @brief Check if character is a blank character.
 * @param c The character to check.
 * @return Non-zero if the character is a blank character, 0 otherwise.
 */
int isblank(int c);

/**
 * @brief Convert character to lowercase.
 * @param c The character to convert.
 * @return The lowercase equivalent of the character, or the character itself if no lowercase equivalent.
 */
int tolower(int c);

/**
 * @brief Convert character to uppercase.
 * @param c The character to convert.
 * @return The uppercase equivalent of the character, or the character itself if no uppercase equivalent.
 */
int toupper(int c);

#endif // __CTYPE_H__