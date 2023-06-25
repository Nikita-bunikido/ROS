#ifndef _ROS_H
#define _ROS_H

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

#define LO8(w)              ((w) & 0xFF)
#define HI8(w)              (((w) >> 8) & 0xFF)
#define AR8(w)              HI8(w), LO8(w)

#define USTR(s)             ((unsigned char *)(s))
#define COMMA               ,

#define ARR_SIZE(a)         (sizeof(a) / sizeof(__typeof__(*a)))

#define INPUT_BUFFER_CAP    260

struct Input_Buffer {
    unsigned short cursor;
    char raw[INPUT_BUFFER_CAP];
};

extern enum System_Mode {
    SYSTEM_MODE_INPUT = 0,  /* Command mode\Listening to input */
    SYSTEM_MODE_BUSY,       /* System is busy and doesn't listen any input */
    SYSTEM_MODE_IDLE,       /* System is waiting for any key to press */
} sys_mode;

/* --------------- Attributes --------------- */
#define PACKED              __attribute__((packed))
#define NOINLINE            __attribute__((noinline))
#define USED                __attribute__((used))
#define USED_NOINLINE       __attribute__((used, noinline))

#define __callback          USED_NOINLINE
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

#define ROS_TOGGLE_PIN(port,pin,ms) \
do {                                \
    BIT_OFF(PORT##port, (pin));     \
    _delay_ms(ms);                  \
    BIT_ON(PORT##port, (pin));      \
    _delay_ms(ms);                  \
} while( 0 )                        \

#define ROS_SET_PIN_DIRECTION(port,pin,dir)     ros_set_pin_direction(&(PORT##port), &(DDR##port), pin, dir)
extern void ros_set_pin_direction(volatile uint8_t *, volatile uint8_t *, int, enum Pin_Direction);

/* --------------- Callbacks --------------- */
extern void __callback keyboard_nonprintable_down_arrow(void);
extern void __callback keyboard_nonprintable_right_arrow(void);
extern void __callback keyboard_nonprintable_up_arrow(void);
extern void __callback keyboard_nonprintable_control(void);
extern void __callback keyboard_nonprintable_left_arrow(void);
extern void __callback keyboard_nonprintable_enter(void);
extern void __callback keyboard_nonprintable_backspace(void);
extern void __callback keyboard_nonprintable_tab(void);

#endif /* _ROS_H */