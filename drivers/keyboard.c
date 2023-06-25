#include <inttypes.h>
#include <ctype.h>

#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "ros.h"
#include "video.h"
#include "keyboard.h"

#ifndef NDEBUG
    /* Speed-up emulator */
    #define KEYBOARD_DELAY_MS       0
#else
    #define KEYBOARD_DELAY_MS       100
#endif

static Keyboard_User_Callback keyboard_callback = NULL;

/* Lookup table for optimization. */
/* Structured as pairs of characters ( without and with SHIFT/CAPS mode ) */
/* If first byte is '\xff', second byte is index of callback function */
static const uint8_t char_decode_table[] PROGMEM =
"\xff\x00" "\xff\x01" "mM,<.>/?" "\xff\x02\xff\x03" "  " "\xff\x04\xff\x05\xff\x06"
"zZxXcCvVbBnNfFgGhHjJkKlL;:\'\"pP[{]}\\|" "\xff\x07" "aAsSdDwWeErRtTyYuUiIoO8*9(0)"
"-_=+\xff\x08\xff\x09qQ`~1!2@3#4$5%6^7&";

static bool shift_mode = false, 
            caps_mode  = false;

static void __callback keyboard_nonprintable_shift(void)    { shift_mode = true; }
static void __callback keyboard_nonprintable_capslock(void) { caps_mode = !caps_mode; }

static const Keyboard_Nonprintable_Callback keyboard_nonprintable_callbacks[0xA] = {
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
    uint16_t kdata = pgm_read_word(&char_decode_table[key * 2]);

    if (LO8(kdata) == 0xFF) {
        if (sys_mode == SYSTEM_MODE_INPUT)
            keyboard_nonprintable_callbacks[HI8(kdata)]();
        return -1; /* Not printable */
    }

    int ret = ((caps_mode && islower(LO8(kdata))) || shift_mode) ? HI8(kdata) : LO8(kdata);    
    shift_mode = false;
    return ret;
}

void __driver keyboard_init(Keyboard_User_Callback callback) {
    cli();
    keyboard_callback = callback;

    ROS_SET_PIN_DIRECTION(C, KEYBOARD_CLK_PIN, PIN_DIRECTION_OUTPUT);
    ROS_SET_PIN_DIRECTION(C, KEYBOARD_SHLD_PIN, PIN_DIRECTION_OUTPUT);
    ROS_SET_PIN_DIRECTION(C, KEYBOARD_SO_PIN, PIN_DIRECTION_INPUT);
    ROS_SET_PIN_DIRECTION(D, KEYBOARD_INTERRUPT_PIN, PIN_DIRECTION_INPUT_PULLUP);

    EIMSK |= BIT(INT0);
    sei();
}

ISR(INT0_vect) {
    if (sys_mode == SYSTEM_MODE_BUSY)
        return;

    ROS_TOGGLE_PIN(C, KEYBOARD_SHLD_PIN, KEYBOARD_DELAY_MS);

    uint64_t keyboard_shot = 0;
    for (int i = 0; i < 58; i++) {
        keyboard_shot = (keyboard_shot << 1) | ((~BIT_EXT(PINC, KEYBOARD_SO_PIN)) & 1);
        ROS_TOGGLE_PIN(C, KEYBOARD_CLK_PIN, KEYBOARD_DELAY_MS);
    }

    int idx = 0;
    for (; keyboard_shot; keyboard_shot >>= 1, idx ++)
        ;

    if (keyboard_callback != NULL)
        keyboard_callback(idx - 1);

    ros_apply_output_entrys();
    ros_put_graphic_cursor();
    ros_apply_output_entrys();
}