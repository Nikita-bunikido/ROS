#ifndef _COMMON_H
#define _COMMON_H

#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <inttypes.h>
#include <stdio.h>

#define COMMA                   ,

#define ANSI_RED                "\033[31m"
#define ANSI_YELLOW             "\033[33m"
#define ANSI_MAGENTA            "\033[35m"
#define ANSI_CYAN               "\033[36m"
#define ANSI_RESET              "\033[0m"

extern volatile struct Warning_Info {
    union {
        struct __attribute__((__packed__)) {
            bool w_error;
            bool w_separate;
            bool w_conversions;
            bool w_range;
        };
        bool w_raw[4];
    };
} warning_info;

typedef struct Position Position;
struct Position {
    unsigned line;
    unsigned character;
};

struct Input {
    const char *file;
    char *base;
    Position pos;
};

#define ASM_UNUSED(x)           (void)(x)
#define ASM_ASSERT(expr, ...)   do{ if(expr) break; __VA_OPT__(__VA_ARGS__;) _asm_assert(#expr, __FILE__, __LINE__); } while( 0 )
static inline __attribute__((noreturn)) void _asm_assert(const char * const expr, const char *file, unsigned line) {
    fprintf(stderr, "%s:%u: " ANSI_RED "ROS-CHIP-8 Assertion failed" ANSI_RESET ": \"%s\"\n", file, line, expr);
    abort();
}
#define ASM_ASSERT_NOT_NULL(x, ...)  do{ ASM_ASSERT((x) != NULL __VA_OPT__(,) __VA_ARGS__); }while( 0 );

#define ASM_WARNING(t, ...)                                                     \
do {                                                                            \
    if (warning_info.w_error)                                                   \
        _asm_error(&(struct Input){(t)->file, NULL, (t)->pos}, __VA_ARGS__);    \
    else                                                                        \
        _asm_warning(&(struct Input){(t)->file, NULL, (t)->pos}, __VA_ARGS__);  \
} while( 0 )                                                                    
static inline __attribute__((format(printf, 2, 3))) void _asm_warning(struct Input *input, const char *fmt, ...) {
    va_list vptr;
    va_start(vptr, fmt);

    fprintf(stderr, "%s:%u:%u: " ANSI_YELLOW "ROS-CHIP-8 warning" ANSI_RESET ": ", input->file, input->pos.line, input->pos.character);

    vfprintf(stderr, fmt, vptr);
    fputc('\n', stderr);

    va_end(vptr);
}

#define ASM_ERROR(t, ...)          _asm_error(&(struct Input){(t)->file, NULL, (t)->pos}, __VA_ARGS__)
static inline __attribute__((noreturn, format(printf, 2, 3))) void _asm_error(struct Input *input, const char *fmt, ...) {
    va_list vptr;
    va_start(vptr, fmt);

    fprintf(stderr, "%s:%u:%u: " ANSI_RED "ROS-CHIP-8 error" ANSI_RESET ": ", input->file, input->pos.line, input->pos.character);

    vfprintf(stderr, fmt, vptr);
    fputc('\n', stderr);

    va_end(vptr);
    exit(EXIT_FAILURE);
}

inline bool ishex(char ch) {
    if (isupper(ch)) ch = tolower(ch);
    return (ch >= 'a') && (ch <= 'f');
}

#endif /* _COMMON_H */