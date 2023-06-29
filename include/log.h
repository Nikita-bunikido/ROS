#ifndef _LOG_H
#define _LOG_H

#include "ros.h"

#define LOG_TYPES_NUMBER    (LOG_TYPE_CRITICAL + 1)
#define HARD_ERROR(code)    ros_log(LOG_TYPE_CRITICAL, NULL, (code))

enum Critical_Code {
    DRIVER_KEYBOARD_FAULT = 0x00,
    VIDEO_MEMORY_FAULT    = 0x10,
};

enum Log_Type {
    LOG_TYPE_INFO = 0,
    LOG_TYPE_ERROR,
    LOG_TYPE_WARNING,
    LOG_TYPE_CRITICAL,
};

void ros_log(enum Log_Type, const char *, ...)
                                            __attribute__((format(printf, 2, 3)));

#endif /* _LOG_H */