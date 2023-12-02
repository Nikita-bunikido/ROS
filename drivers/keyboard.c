#include <inttypes.h>
#include <ctype.h>

#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include <drivers/spi.h>
#include <drivers/keyboard.h>

#include <ros/video.h>
#include <ros/log.h>
#include <ros/ros-for-modules.h>

#ifndef NDEBUG
    /* Speed-up emulator */
    #define KEYBOARD_DELAY_MS       0
#else
    #define KEYBOARD_DELAY_MS       100
#endif

static volatile Keyboard_User_Callback input_keyboard_callback = NULL;

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

static const Keyboard_Nonprintable_Callback keyboard_nonprintable_callbacks[] = {
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

volatile enum Virtual_Key idle_key = INVALID_KEY;

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

void __driver keyboard_init(Keyboard_User_Callback input_callback) {
    cli();
    input_keyboard_callback = input_callback;

    ROS_SET_PIN_DIRECTION(C, KEYBOARD_CLK_PIN, PIN_DIRECTION_OUTPUT);
    ROS_SET_PIN_DIRECTION(C, KEYBOARD_SHLD_PIN, PIN_DIRECTION_OUTPUT);
    ROS_SET_PIN_DIRECTION(C, KEYBOARD_SO_PIN, PIN_DIRECTION_INPUT);
    ROS_SET_PIN_DIRECTION(D, KEYBOARD_INTERRUPT_PIN, PIN_DIRECTION_INPUT_PULLUP);

    EIMSK |= BIT(INT0);                         /* Enable INT0 */
    EICRA |= BIT(ISC01) | BIT(ISC00);           /* Rising edge mode */
    sei();
}

ISR(INT0_vect) {
    while (!(SPI->rSPSR & BIT(SPIF))) /* Wait for spi transaction to complete */
        ;

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

    /* 74hc165 fault */
    if ((idx - 1) >= 58)
        HARD_ERROR(FAULT_DRIVER_KEYBOARD);

    switch (sys_mode) {
    case SYSTEM_MODE_INPUT:
        if (input_keyboard_callback) input_keyboard_callback(idx - 1);
        break;

    case SYSTEM_MODE_IDLE:
        idle_key = (enum Virtual_Key)(idx - 1);
        sys_mode = SYSTEM_MODE_BUSY;
        break;
    }
}