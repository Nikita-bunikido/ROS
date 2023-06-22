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

#define ARR_SIZE(a)         (sizeof(a) / sizeof(__typeof__(*a)))

/* --------------- Attributes --------------- */
#define PACKED              __attribute__((packed))
#define NOINLINE            __attribute__((noinline))
#define USED                __attribute__((used))
#define USED_NOINLINE       __attribute__((used, noinline))

#define __callback          USED_NOINLINE
#define __driver            NOINLINE

/* -------------- Screen --------------------- */
#define SCREEN_WIDTH        (128 - 1)
#define SCREEN_HEIGHT       (160 - 1)

/* --------------- Directions --------------- */
enum Pin_Direction {
    PIN_DIRECTION_INPUT = 0,
    PIN_DIRECTION_INPUT_PULLUP,
    PIN_DIRECTION_OUTPUT,
};

extern void ros_set_pin_direction(volatile uint8_t *, volatile uint8_t *, int, enum Pin_Direction);
#define ROS_SET_PIN_DIRECTION(port,pin,dir)     ros_set_pin_direction(&(PORT##port), &(DDR##port), pin, dir)

#endif /* _ROS_H */