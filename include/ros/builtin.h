#ifndef _BUILTIN_H
#define _BUILTIN_H

#include <inttypes.h>

#include <ros/ros-for-headers.h>

#define PROGRAM_NAME_MAX    32U

struct PACKED Builtin_Program {
    char name[PROGRAM_NAME_MAX];
    void *raw;
    size_t size;
};

int get_builtin_program(struct Builtin_Program *, const char *);

#endif /* _BUILTIN_H */