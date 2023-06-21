#include <inttypes.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "ros.h"
#include "keyboard.h"

#define KEYBOARD_DELAY_MS       0

static Keyboard_User_Callback keyboard_callback = NULL;

static void __callback keyboard_nonprintable_down_arrow(void) {
    __asm__ __volatile__( "nop" );
}

static void __callback keyboard_nonprintable_right_arrow(void) {
    __asm__ __volatile__( "nop" );
}

static void __callback keyboard_nonprintable_up_arrow(void) {
    __asm__ __volatile__( "nop" );
}

static void __callback keyboard_nonprintable_control(void) {
    __asm__ __volatile__( "nop" );
}

static void __callback keyboard_nonprintable_left_arrow(void) {
    __asm__ __volatile__( "nop" );
}

static void __callback keyboard_nonprintable_enter(void) {
    __asm__ __volatile__( "nop" );
}

static void __callback keyboard_nonprintable_shift(void) {
    __asm__ __volatile__( "nop" );
}

static void __callback keyboard_nonprintable_capslock(void) {
    __asm__ __volatile__( "nop" );
}

static void __callback keyboard_nonprintable_backspace(void) {
    __asm__ __volatile__( "nop" );
}

static void __callback keyboard_nonprintable_tab(void) {
    __asm__ __volatile__( "nop" );
}

/* This is lookup table for optimization, structured as pairs of characters */
/* without SHIFT mode and with SHIFT mode. Pairs starting with \xff are indexes */
/* for following functions to process non-printable characters. */
static const uint8_t char_decode_table[] PROGMEM =
"\xff\x00" "\xff\x01" "mM,<.>/?" "\xff\x02\xff\x03" "  " "\xff\x04\xff\x05\xff\x06"
"zZxXcCvVbBnNfFgGhHjJkKlL;:\'\"pP[{]}\\|" "\xff\x07" "aAsSdDwWeErRtTyYuUiIoO8*9(0)"
"-_=+\xff\x08\xff\x09qQ`~1!2@3#4$5%6^7&";

static bool shift_mode = false, caps_mode = false;

static const Keyboard_Nonprintable_Callback keyboard_nonprintable_callbacks[10] = {
    keyboard_nonprintable_down_arrow,
    keyboard_nonprintable_right_arrow,
    keyboard_nonprintable_up_arrow,
    keyboard_nonprintable_control,
    keyboard_nonprintable_left_arrow,
    keyboard_nonprintable_enter,
    keyboard_nonprintable_shift,
    keyboard_nonprintable_capslock,
    keyboard_nonprintable_backspace,
    keyboard_nonprintable_tab,
};

int vk_as_char(enum Virtual_Key key) {
    if (key == VK_SHIFT) {
        shift_mode = true;
        return -1; /* Not printable */
    } else if (key == VK_CAPSLOCK) {
        caps_mode = !caps_mode;
        return -1; /* Not printable */
    }

    uint16_t kdata = pgm_read_word(&char_decode_table[key * 2]);

    if (LO8(kdata) == 0xFF) {
        keyboard_nonprintable_callbacks[HI8(kdata)]();
        return -1; /* Not printable */
    }

    int ret = ((caps_mode && islower(LO8(kdata))) || shift_mode) ? HI8(kdata) : LO8(kdata);    
    shift_mode = false;
    return ret;
}

void keyboard_init(Keyboard_User_Callback callback) {
    keyboard_callback = callback;

    ROS_SET_PIN_DIRECTION(C, KEYBOARD_CLK_PIN, PIN_DIRECTION_OUTPUT);
    ROS_SET_PIN_DIRECTION(C, KEYBOARD_SHLD_PIN, PIN_DIRECTION_OUTPUT);
    ROS_SET_PIN_DIRECTION(C, KEYBOARD_SO_PIN, PIN_DIRECTION_INPUT);
    ROS_SET_PIN_DIRECTION(D, 2, PIN_DIRECTION_INPUT_PULLUP);

    EIMSK |= BIT(INT0);
}

ISR(INT0_vect) {
    BIT_OFF(PORTC, KEYBOARD_SHLD_PIN);
    _delay_ms(KEYBOARD_DELAY_MS);
    BIT_ON(PORTC, KEYBOARD_SHLD_PIN);
    _delay_ms(KEYBOARD_DELAY_MS);

    uint64_t keyboard_shot = 0;
    for (int i = 0; i < 58; i++){
        keyboard_shot = (keyboard_shot << 1) | ((~BIT_EXT(PINC, KEYBOARD_SO_PIN)) & 1);

        BIT_OFF(PORTC, KEYBOARD_CLK_PIN);
        _delay_ms(KEYBOARD_DELAY_MS);
        BIT_ON(PORTC, KEYBOARD_CLK_PIN);
        _delay_ms(KEYBOARD_DELAY_MS);
    }

    int idx = 0;
    for(; keyboard_shot; keyboard_shot >>= 1, idx ++)
        ;

    if (keyboard_callback != NULL)
        keyboard_callback(idx - 1);
}