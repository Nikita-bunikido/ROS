#ifndef _VIDEO_H
#define _VIDEO_H

#include <inttypes.h>
#include <assert.h>

#include "ros.h"

typedef struct {
    uint8_t x, y;
} v2;

struct PACKED Attribute {
    unsigned int fore_r   : 1;
    unsigned int fore_g   : 1;
    unsigned int fore_b   : 1;
    unsigned int reserved : 1;
    unsigned int back_r   : 1;
    unsigned int back_g   : 1;
    unsigned int back_b   : 1;
    unsigned int blink    : 1;
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

#ifndef NDEBUG
    void ros_apply_output_entrys(void);
#endif

inline uint8_t struct_attribute_to_raw(struct Attribute attr)
{ return *(uint8_t *)&attr; }

extern void __callback keyboard_nonprintable_down_arrow(void);
extern void __callback keyboard_nonprintable_right_arrow(void);
extern void __callback keyboard_nonprintable_up_arrow(void);
extern void __callback keyboard_nonprintable_control(void);
extern void __callback keyboard_nonprintable_left_arrow(void);
extern void __callback keyboard_nonprintable_enter(void);
extern void __callback keyboard_nonprintable_backspace(void);
extern void __callback keyboard_nonprintable_tab(void);

#endif /* _VIDEO_H */