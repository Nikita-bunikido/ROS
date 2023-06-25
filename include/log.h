#ifndef _LOG_H
#define _LOG_H

#include "ros.h"

#define LOG_TYPES_NUMBER    (LOG_TYPE_CRITICAL + 1)
#define LOG_TYPE_CSTR       (const char*[LOG_TYPES_NUMBER]){"INFO", "FAIL", "WARN", ""}

enum Log_Type {
    LOG_TYPE_INFO = 0,
    LOG_TYPE_ERROR,
    LOG_TYPE_WARNING,
    LOG_TYPE_CRITICAL,
};

void ros_log(enum Log_Type, const char *, ...)
                                            __attribute__((format(printf, 2, 3)));

#endif /* _LOG_H */