#ifndef __EMOS_STATUS_H__
#define __EMOS_STATUS_H__

#include <emos/types.h>

typedef unsigned int status_t;

#define CHECK_SUCCESS(status) (!((status) & 0x80000000))
#define CHECK_FAILURE(status) (!!((status) & 0x80000000))

#define CHECK_NORMAL_FAILURE(status) \
    (CHECK_FAILURE(status) && (!((status) & 0x40000000)))
#define CHECK_CRITICAL_FAILURE(status) \
    (CHECK_FAILURE(status) && (!!((status) & 0x40000000)))

#define STATUS_SUCCESS              0x00000000

#define STATUS_UNKNOWN_ERROR        0x80000000
#define STATUS_INVALID_OBJECT       0x80000001
#define STATUS_INVALID_RESOURCE     0x80000002
#define STATUS_UNEXPECTED_RESULT    0x80000003
#define STATUS_INVALID_SIGNATURE    0x80000004
#define STATUS_INVALID_VALUE        0x80000005

#define STATUS_FS_INCONSISTENT      0xC0000000

#endif // __EMOS_STATUS_H__
