#ifndef _VIDEO_H
#define _VIDEO_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>

#include "ros.h"

#define TIMER0_PRESCALER    1024

typedef struct {
    uint8_t x, y;
} v2;

enum Attribute_Preset {
    ATTRIBUTE_DEFAULT   = 0x7,
    ATTRIBUTE_UNDERLINE = 0xF
};

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
static_assert( sizeof(struct Attribute) == 1 );

struct PACKED Output_Entry {
    v2 pos;
    union {
        struct Attribute attrib;
        uint8_t attrib_raw;
    };
    unsigned char data;
};

extern volatile struct PACKED Graphic_Cursor {
    uint8_t attrib_low, attrib_high;
    bool visible;
} graphic_cursor;

unsigned char ros_putchar(uint8_t, const unsigned char);
int ros_puts(uint8_t, const unsigned char *, bool);
int ros_puts_P(uint8_t, const unsigned char *, bool);
int ros_vprintf(uint8_t, const char *, va_list);
int ros_printf(uint8_t, const char *, ...) __attribute__((format(printf, 2, 3)));

void ros_put_input_buffer(unsigned short, int);
void ros_put_prompt(void);

void ros_apply_output_entrys(void);
void clear_screen(uint16_t);

void ros_graphic_timer_init(void);

inline uint8_t struct_attribute_to_raw(struct Attribute attr)
{ return *(uint8_t *)&attr; }

inline void enable_cursor(void)
{ graphic_cursor.visible = true; }

inline void disable_cursor(void)
{ graphic_cursor.visible = false; }

#endif /* _VIDEO_H */