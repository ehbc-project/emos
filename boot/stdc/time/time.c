#include <time.h>

time_t time(time_t *time)
{
    return 0;
}

time_t mktime(struct tm *tm)
{
    return 0;
}

struct tm *gmtime(const time_t *time)
{
    static struct tm tim = { 0, };
    return &tim;
}

struct tm *localtime(const time_t *time)
{
    static struct tm tim = { 0, };
    return &tim;
}

size_t strftime(char *__restrict str, size_t maxsize, const char *__restrict fmt, const struct tm *__restrict tm)
{
    return 0;
}

char *asctime(const struct tm *tm)
{
    return NULL;
}

char *ctime(const time_t *timer)
{
    return NULL;
}
