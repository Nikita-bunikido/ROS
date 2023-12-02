#ifndef _VIDEO_H
#define _VIDEO_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>

#include <ros/ros-for-headers.h>

#define TIMER0_PRESCALER    1024
#define VGA_SWITCH(a)       (a) = (((a) & 0x88) | (((a) & 0x7) << 0x4) | (((a) & 0x70) >> 4))

typedef struct {
    uint8_t x, y;
} v2;

typedef void (*__callback Flash_Routine)(bool flash_flag);

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

struct PACKED Flash_Thread {
    Flash_Routine handle;
    v2 pos;
};

struct PACKED Running_String_Info {
    const unsigned char *raw;
    union {
        struct Attribute attrib;
        uint8_t attrib_raw;
    };
    uint8_t len;
    uint8_t offset;
};

extern volatile struct PACKED Graphic_Cursor {
    uint8_t attrib_low, attrib_high;
    bool visible;
} graphic_cursor;

/* --------------- Initializers --------------- */
void ros_graphic_timer_init(void);

/* --------------- General I/O --------------- */
unsigned char ros_putchar(uint8_t, const unsigned char);
int ros_puts(uint8_t, const unsigned char *, bool);
int ros_puts_P(uint8_t, const unsigned char *, bool);
int ros_vprintf(uint8_t, const char *, va_list);
int ros_printf(uint8_t, const char *, ...) __attribute__((format(printf, 2, 3)));
int ros_puts_R(const struct Running_String_Info * const);
void ros_flash(Flash_Routine);

/* --------------- Misc --------------- */
void ros_put_input_buffer(unsigned short, int);
void ros_put_prompt(void);
void clear_screen(uint16_t);
void enable_cursor(void);
void disable_cursor(void);
void set_cursor_position(v2 pos);

/* --------------- Other --------------- */
void ros_apply_output_entrys(void);

inline uint8_t struct_attribute_to_raw(struct Attribute attr)
{ return *(uint8_t *)&attr; }

#endif /* _VIDEO_H */