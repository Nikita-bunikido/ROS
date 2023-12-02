#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <drivers/memory.h>
#include <drivers/keyboard.h>

#include <ros/ros-for-modules.h>

#define ADDRESS_LEN               15

#ifndef NDEBUG
    /* Speed-up emulator */
    #define MEMORY_DELAY_MS       0
#else
    #define MEMORY_DELAY_MS       100
#endif

static void set_address_bus(Address addr) {
    for (int i = 0; i < ADDRESS_LEN; i ++, addr >>= 1) {
        ROS_TOGGLE_PIN(C, MEMORY_CLK_PIN, MEMORY_DELAY_MS);
        BIT_SET(PORTC, MEMORY_DS_PIN, addr & 1);
    }
}

static void set_data_bus(const uint8_t data) {
    BIT_SET(PORTC, 3, data & 1);
    PORTD = ((data >> 1) & 3) | (BIT_EXT(PORTD, 2) << 2) | (((data >> 3) & 0x1F) << 3);
}

static uint8_t read_data_bus(void) {
    return BIT_EXT(PINC, 3) | ((PIND & 3) << 1) | ((PIND & 0xF8));
}

void memory_init(void) {
    cli();
    ROS_SET_PIN_DIRECTION(C, MEMORY_CLK_PIN, PIN_DIRECTION_OUTPUT);
    ROS_SET_PIN_DIRECTION(C, MEMORY_DS_PIN, PIN_DIRECTION_OUTPUT);
    ROS_SET_PIN_DIRECTION(B, MEMORY_RW_PIN, PIN_DIRECTION_OUTPUT);
    BIT_ON(PORTB, MEMORY_RW_PIN);

    for (int i = 0; i <= 7; ++i) {
        if  (i == KEYBOARD_INTERRUPT_PIN) continue;
        ROS_SET_PIN_DIRECTION(D, i, PIN_DIRECTION_OUTPUT);
    }

    ROS_SET_PIN_DIRECTION(C, 3, PIN_DIRECTION_OUTPUT);
    sei();
}

uint8_t memory_write(Address addr, uint8_t data) {
    set_address_bus(addr);
    set_data_bus(data);

    BIT_OFF(PORTB, MEMORY_RW_PIN);
    BIT_ON(PORTB, MEMORY_RW_PIN);
    return data;
}

uint8_t memory_read(Address addr) {
    set_address_bus(addr);

    for (int i = 0; i <= 7; ++i) {
        if (i == KEYBOARD_INTERRUPT_PIN) continue;
        ROS_SET_PIN_DIRECTION(D, i, PIN_DIRECTION_INPUT_PULLUP);
    }
    ROS_SET_PIN_DIRECTION(C, 3, PIN_DIRECTION_INPUT_PULLUP);
    BIT_ON(PORTB, MEMORY_RW_PIN);

    return read_data_bus();
}

void memory_write_buffer(Address addr, const void *data, size_t size) {
    if (!size)
        return;
    
    while (size-- > 0)
        memory_write(addr ++, *(uint8_t *)data ++);
}

void memory_write_buffer_P(Address addr, const void *data, size_t size) {
    if (!size)
        return;

    for (const uint8_t *datap = data; size-- > 0; datap ++)
        memory_write(addr ++, pgm_read_byte(datap));
}