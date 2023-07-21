#ifndef _LOG_H
#define _LOG_H

#include "ros.h"

#define LOG_TYPES_NUMBER    (LOG_TYPE_CRITICAL + 1)
#define HARD_ERROR(code)    ros_log(LOG_TYPE_CRITICAL, NULL, (code))

enum Critical_Code {
    FAULT_DRIVER_KEYBOARD      = 0x00,
    FAULT_KERNEL_BAD_INTERRUPT = 0x10,
    FAULT_KERNEL_ZERO_DIVIDER  = 0x11,
    FAULT_VIDEO_MEMORY         = 0x20,
    FAULT_RAM_MEMORY           = 0x30,
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