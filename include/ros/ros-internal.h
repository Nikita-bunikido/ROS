#ifndef _ROS_INTERNAL_H
#define _ROS_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>
#include <assert.h>

#include <avr/io.h>

/* --------------- Common --------------- */
#define BIT(x)              _BV(x)
#define BIT_OFF(x, bit)     (x) = ((x) & ~BIT(bit))
#define BIT_ON(x, bit)      (x) = ((x) | BIT(bit))
#define BIT_EXT(x, bit)     (((x) >> (bit)) & 1)
#define BIT_SET(x, bit, d)  do { BIT_OFF(x,bit); if(!(d)) break; BIT_ON(x,bit); }while( 0 )

#define LO8(w)              ((w) & 0xFF)
#define HI8(w)              (((w) >> 8) & 0xFF)
#define AR8(w)              HI8(w), LO8(w)

#define USTR(s)             ((unsigned char *)(s))
#define UCHR(c)             ((unsigned char)(c))
#define COMMA               ,

#define ARR_SIZE(a)         (sizeof(a) / sizeof(__typeof__(*a)))

#define INPUT_BUFFER_CAP    260
#define INPUT_BUFFER_NARGS  128

struct Input_Buffer {
    unsigned short cursor;
    char raw[INPUT_BUFFER_CAP];
};

struct Argument {
    unsigned short len;
    char *base;
};

extern enum System_Mode {
    SYSTEM_MODE_INPUT = 0,  /* Command mode\Listening to input */
    SYSTEM_MODE_BUSY,       /* System is busy and doesn't listen any input */
    SYSTEM_MODE_IDLE,       /* System is waiting for any key to press */
} sys_mode;

/* --------------- Compiler specific --------------- */
#define UNREACHABLE()       __builtin_unreachable()
#define NOP()               asm volatile ("nop")

#define BSWAP16(_x)         __extension__ ({    \
    __typeof__(_x) __x = (_x);                  \
    __builtin_bswap16(__x);                     \
})

#define PACKED              __attribute__((__packed__))
#define NOINLINE            __attribute__((__noinline__))
#define USED                __attribute__((__used__))
#define USED_NOINLINE       __attribute__((__used__, __noinline__))
#define ALWAYS_INLINE       __attribute__((__always_inline__))

#if defined(_ROS_HEADER)
#   define __callback          USED
#else
#   define __callback          USED_NOINLINE
#endif /* _ROS_HEADER */
#define __driver            NOINLINE

/* --------------- Screen --------------- */
#define SCREEN_WIDTH        (128 - 1)
#define SCREEN_HEIGHT       (160 - 1)

/* --------------- Ports --------------- */
enum Pin_Direction {
    PIN_DIRECTION_INPUT = 0,
    PIN_DIRECTION_INPUT_PULLUP,
    PIN_DIRECTION_OUTPUT,
};

#define ROS_TOGGLE_PIN(port,pin,ms)             \
do {                                            \
    BIT_OFF(PORT##port, (pin));                 \
    _delay_ms(ms);                              \
    BIT_ON(PORT##port, (pin));                  \
    _delay_ms(ms);                              \
} while( 0 )                                    \

#define ROS_SET_PIN_DIRECTION(port,pin,dir)     ros_set_pin_direction(&(PORT##port), &(DDR##port), pin, dir)
extern void ros_set_pin_direction(volatile uint8_t *, volatile uint8_t *, int, enum Pin_Direction);

/* --------------- Callbacks --------------- */
typedef void __callback Keyboard_Callback(void);
extern Keyboard_Callback keyboard_nonprintable_down_arrow,
                         keyboard_nonprintable_right_arrow,
                         keyboard_nonprintable_up_arrow,
                         keyboard_nonprintable_control,
                         keyboard_nonprintable_left_arrow,
                         keyboard_nonprintable_enter,
                         keyboard_nonprintable_backspace,
                         keyboard_nonprintable_tab;

/* --------------- Misc --------------- */
extern int caseless_cmp(const char *, const char *);

#endif /* _ROS_INTERNAL_H */