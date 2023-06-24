#ifndef _VIDEO_H
#define _VIDEO_H

#include <inttypes.h>
#include <assert.h>

#include "ros.h"

typedef struct {
    uint8_t x, y;
} v2;

struct PACKED Attribute {
    unsigned int fore_r    : 1;
    unsigned int fore_g    : 1;
    unsigned int fore_b    : 1;
    unsigned int underline : 1;
    unsigned int back_r    : 1;
    unsigned int back_g    : 1;
    unsigned int back_b    : 1;
    unsigned int blink     : 1;
};
static_assert(sizeof(struct Attribute) == 1);

struct PACKED Output_Entry {
    v2 pos;
    union {
        struct Attribute attrib;
        uint8_t attrib_raw;
    };
    unsigned char data;
};

void ros_printf(uint8_t, const char *, ...) __attribute__((format(printf, 2, 3)));
void ros_putchar(uint8_t, const char);
void ros_puts(uint8_t, const char *, bool);
void ros_puts_P(uint8_t, const unsigned char *, bool);

void ros_apply_output_entrys(void);

void ros_put_input_buffer(int, int);

void ros_prompt(void);

void draw_graphic_cursor(void);
void clear_screen(void);

inline uint8_t struct_attribute_to_raw(struct Attribute attr)
{ return *(uint8_t *)&attr; }

#endif /* _VIDEO_H */