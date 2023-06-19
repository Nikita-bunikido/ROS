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

#define PACKED              __attribute__((packed))
#define NOINLINE            __attribute__((noinline))
#define USED                __attribute__((used))
#define USED_NOINLINE       __attribute__((used, noinline))

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

/* --------------- IO --------------- */
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
    void ros_output_entry_push(struct Output_Entry);
    void ros_apply_output_entrys(void);
#endif

inline uint8_t struct_attribute_to_raw(struct Attribute attr)
{ return *(uint8_t *)&attr; }

#endif /* _ROS_H */