#ifndef __TIME_H__
#define __TIME_H__

#include <stdint.h>
#include <stddef.h>

#define CLOCKS_PER_SEC 1024

typedef int64_t clock_t;
typedef int64_t time_t;

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

/**
 * @brief Get processor time.
 * @return The processor time used by the program since its launch.
 */
clock_t clock(void);

/**
 * @brief Get current time.
 * @param time A pointer to a time_t object to store the current time, or NULL.
 * @return The current calendar time.
 */
time_t time(time_t *time);

/**
 * @brief Convert tm structure to time_t.
 * @param tm A pointer to a tm structure.
 * @return The calendar time encoded as a time_t object.
 */
time_t mktime(struct tm *tm);

/**
 * @brief Convert time_t to UTC tm structure.
 * @param time A pointer to a time_t object.
 * @return A pointer to a tm structure representing the UTC time.
 */
struct tm *gmtime(const time_t *time);

/**
 * @brief Convert time_t to local tm structure.
 * @param time A pointer to a time_t object.
 * @return A pointer to a tm structure representing the local time.
 */
struct tm *localtime(const time_t *time);

/**
 * @brief Format time as a string.
 * @param str A pointer to the buffer to store the formatted string.
 * @param maxsize The maximum number of characters to write.
 * @param fmt The format string.
 * @param tm A pointer to a tm structure.
 * @return The number of characters written, or 0 if an error occurred.
 */
size_t strftime(char *str, size_t maxsize, const char *fmt, const struct tm *tm);

/**
 * @brief Convert tm structure to string.
 * @param tm A pointer to a tm structure.
 * @return A pointer to a string representing the time.
 */
char *asctime(const struct tm *tm);

/**
 * @brief Convert time_t to string.
 * @param timer A pointer to a time_t object.
 * @return A pointer to a string representing the time.
 */
char *ctime(const time_t *timer);


#endif // __TIME_H__
