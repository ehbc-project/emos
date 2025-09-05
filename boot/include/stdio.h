#ifndef __STDIO_H__
#define __STDIO_H__

#include <stdarg.h>
#include <stddef.h>

#define PATH_MAX 4096
#define FILENAME_MAX 256

# ifndef PRINTF_MAX_LEN
#   define PRINTF_MAX_LEN 1024

# endif

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#define EOF         -1

struct _iobuf {
    struct filesystem *fs;
    struct fs_file *file;

    struct device *dev;
    const struct char_interface *charif;
};

typedef struct _iobuf FILE;

extern FILE *stdin, *stdout, *stderr, *stddbg;

/**
 * @brief Write formatted output to a string.
 * @param buf The buffer to write to.
 * @param fmt The format string.
 * @param ... The arguments to format.
 * @return The number of characters written, or a negative value if an error occurred.
 */
__attribute__((format(printf, 2, 3)))
int sprintf(char *buf, const char *fmt, ...);

/**
 * @brief Write formatted output to a sized buffer.
 * @param buf The buffer to write to.
 * @param size The maximum number of characters to write.
 * @param fmt The format string.
 * @param ... The arguments to format.
 * @return The number of characters written, or a negative value if an error occurred.
 */
__attribute__((format(printf, 3, 4)))
int snprintf(char *buf, size_t size, const char *fmt, ...);

/**
 * @brief Write formatted output to stdout.
 * @param fmt The format string.
 * @param ... The arguments to format.
 * @return The number of characters written, or a negative value if an error occurred.
 */
__attribute__((format(printf, 1, 2)))
int printf(const char *fmt, ...);

/**
 * @brief Write formatted output to a stream.
 * @param stream The file stream to write to.
 * @param fmt The format string.
 * @param ... The arguments to format.
 * @return The number of characters written, or a negative value if an error occurred.
 */
__attribute__((format(printf, 2, 3)))
int fprintf(FILE *stream, const char *fmt, ...);

/**
 * @brief Write formatted output to a string from a va_list.
 * @param buf The buffer to write to.
 * @param fmt The format string.
 * @param args The va_list of arguments.
 * @return The number of characters written, or a negative value if an error occurred.
 */
int vsprintf(char *buf, const char *fmt, va_list args);

/**
 * @brief Write formatted output to a sized buffer from a va_list.
 * @param buf The buffer to write to.
 * @param size The maximum number of characters to write.
 * @param fmt The format string.
 * @param args The va_list of arguments.
 * @return The number of characters written, or a negative value if an error occurred.
 */
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

/**
 * @brief Write formatted output to stdout from a va_list.
 * @param fmt The format string.
 * @param args The va_list of arguments.
 * @return The number of characters written, or a negative value if an error occurred.
 */
int vprintf(const char *fmt, va_list args);

/**
 * @brief Write formatted output to a stream from a va_list.
 * @param fp The file stream to write to.
 * @param fmt The format string.
 * @param args The va_list of arguments.
 * @return The number of characters written, or a negative value if an error occurred.
 */
int vfprintf(FILE *fp, const char *fmt, va_list args);

/**
 * @brief Write a character to stdout.
 * @param ch The character to write.
 * @return The character written, or EOF if an error occurred.
 */
int putchar(int ch);

/**
 * @brief Write a string to stdout, followed by a newline.
 * @param str The string to write.
 * @return A non-negative value on success, or EOF if an error occurred.
 */
int puts(const char *str);

/**
 * @brief Write a character to a stream.
 * @param ch The character to write.
 * @param stream The file stream to write to.
 * @return The character written, or EOF if an error occurred.
 */
int putc(int ch, FILE *stream);

/**
 * @brief Write a string to a stream.
 * @param str The string to write.
 * @param stream The file stream to write to.
 * @return A non-negative value on success, or EOF if an error occurred.
 */
int fputs(const char *str, FILE *stream);

/**
 * @brief Read a character from a stream.
 * @param stream The file stream to read from.
 * @return The character read, or EOF if end-of-file or an error occurred.
 */
int fgetc(FILE *stream);

/**
 * @brief Push a character back onto a stream.
 * @param ch The character to push back.
 * @param stream The file stream.
 * @return The character pushed back, or EOF if an error occurred.
 */
int ungetc(int ch, FILE *stream);

/**
 * @brief Read a line from a stream.
 * @param str The buffer to store the line.
 * @param num The maximum number of characters to read.
 * @param stream The file stream to read from.
 * @return A pointer to the buffer, or NULL if end-of-file or an error occurred.
 */
char *fgets(char *str, int num, FILE *stream);

/**
 * @brief Read a line from stdin.
 * @param str The buffer to store the line.
 * @return A pointer to the buffer, or NULL if end-of-file or an error occurred.
 */
char *gets(char *str);

/**
 * @brief Open a file.
 * @param path The name of the file to open.
 * @param mode The file access mode.
 * @return A pointer to the FILE object, or NULL if the file could not be opened.
 */
FILE *fopen(const char *path, const char *mode);

/**
 * @brief Reopen a stream with a new device.
 * @param stream The file stream to reopen.
 * @param device_name The name of the device to associate with the stream.
 * @return 0 on success, or EOF if an error occurred.
 */
int freopen_device(FILE *stream, const char *device_name);

/**
 * @brief Reopen a stream with a new file or mode.
 * @param path The name of the file to open.
 * @param mode The file access mode.
 * @param stream The file stream to reopen.
 * @return A pointer to the FILE object, or NULL if the file could not be opened.
 */
FILE *freopen(const char *path, const char *mode, FILE *stream);

/**
 * @brief Close a file stream.
 * @param stream The file stream to close.
 * @return 0 on success, or EOF if an error occurred.
 */
int fclose(FILE *stream);

/**
 * @brief Flush a file stream.
 * @param stream The file stream to flush.
 * @return 0 on success, or EOF if an error occurred.
 */
int fflush(FILE *stream);

/**
 * @brief Read data from a file stream.
 * @param ptr A pointer to the buffer to store the read data.
 * @param size The size of each element to read.
 * @param count The number of elements to read.
 * @param stream The file stream to read from.
 * @return The number of elements successfully read.
 */
size_t fread(void *ptr, size_t size, size_t count, FILE *stream);

/**
 * @brief Write data to a file stream.
 * @param ptr A pointer to the buffer containing the data to write.
 * @param size The size of each element to write.
 * @param count The number of elements to write.
 * @param stream The file stream to write to.
 * @return The number of elements successfully written.
 */
size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream);

/**
 * @brief Set the file position for a stream.
 * @param stream The file stream.
 * @param offset The offset to seek to.
 * @param origin The origin for the seek operation (SEEK_SET, SEEK_CUR, SEEK_END).
 * @return 0 on success, or a non-zero value if an error occurred.
 */
int fseek(FILE *stream, long offset, int origin);

/**
 * @brief Get the current file position for a stream.
 * @param stream The file stream.
 * @return The current file position, or -1 if an error occurred.
 */
long ftell(FILE *stream);

/**
 * @brief Check if the end-of-file indicator is set for a stream.
 * @param stream The file stream.
 * @return Non-zero if the end-of-file indicator is set, 0 otherwise.
 */
int feof(FILE *stream);

/**
 * @brief Clear the end-of-file and error indicators for a stream.
 * @param stream The file stream.
 */
void clearerr(FILE *stream);

/**
 * @brief Check if the error indicator is set for a stream.
 * @param stream The file stream.
 * @return Non-zero if the error indicator is set, 0 otherwise.
 */
int ferror(FILE *stream);

/**
 * @brief Print a system error message to stderr.
 * @param str A custom message to print before the system error message.
 */
void perror(const char *str);

#endif // __STDIO_H__