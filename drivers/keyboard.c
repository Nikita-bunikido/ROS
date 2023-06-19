#include <inttypes.h>

#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "ros.h"
#include "keyboard.h"

#define U8(x)                   (uint8_t)(x)

#define KEYBOARD_DELAY_MS       1

static void (*keyboard_callback)(unsigned char) = NULL;

void keyboard_init(void (*callback)(unsigned char)){
    keyboard_callback = callback;

    ROS_SET_PIN_DIRECTION(C, KEYBOARD_CLK_PIN, PIN_DIRECTION_OUTPUT);
    ROS_SET_PIN_DIRECTION(C, KEYBOARD_SHLD_PIN, PIN_DIRECTION_OUTPUT);
    ROS_SET_PIN_DIRECTION(C, KEYBOARD_SO_PIN, PIN_DIRECTION_INPUT);
    ROS_SET_PIN_DIRECTION(D, 2, PIN_DIRECTION_INPUT_PULLUP);

    EIMSK |= BIT(INT0);
}

static const uint8_t virtual_keys[] PROGMEM = {
    U8('7'),
    U8('6'),
    U8('5'),
    U8('4'),
    U8('3'),
    U8('2'),
    U8('1'),
    U8('`'),
};

ISR(INT0_vect) {
    BIT_OFF(PORTC, KEYBOARD_SHLD_PIN);
    _delay_ms(KEYBOARD_DELAY_MS);
    BIT_ON(PORTC, KEYBOARD_SHLD_PIN);
    _delay_ms(KEYBOARD_DELAY_MS);

    uint8_t keyboard_shot = 0;
    for (int i = 0; i < 8; i++){
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
        keyboard_callback(pgm_read_byte(&virtual_keys[idx - 1]));
}