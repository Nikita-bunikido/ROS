#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include <avr/io.h>
#include <avr/pgmspace.h>

#include "spi.h"
#include "st7735.h"

#include "video.h"
#include "keyboard.h"
#include "ros.h"

enum System_Mode sys_mode = SYSTEM_MODE_BUSY;

void ros_set_pin_direction(volatile uint8_t *port, volatile uint8_t *ddr, int pin, enum Pin_Direction dir) {
    switch (dir){
    case PIN_DIRECTION_INPUT:
        BIT_OFF(*ddr, pin);
        break;

    case PIN_DIRECTION_INPUT_PULLUP:
        BIT_ON(*port, pin);
        BIT_OFF(*ddr, pin);
        break;

    case PIN_DIRECTION_OUTPUT:
        BIT_OFF(*port, pin);
        BIT_ON(*ddr, pin);
        break;                     

    default:
        break;
    }
}

static char input_buffer[INPUT_BUFFER_CAP] = { 0 };
static int input_cursor = 0;

void keyboard_callback(enum Virtual_Key vk){
    int ch = vk_as_char(vk);

    if ((ch < 0) || (input_cursor >= INPUT_BUFFER_CAP - 1))
        return;

    memmove(input_buffer + input_cursor + 1, input_buffer + input_cursor, INPUT_BUFFER_CAP - input_cursor - 1);
    input_buffer[input_cursor ++] = (char)ch;

    ros_cursor_copy(input_buffer + input_cursor - 1, input_cursor - 1, 0);
    ros_apply_output_entrys();
}

/* Keyboard callbacks are only active in SYSTEM_MODE_INPUT */
void __callback keyboard_nonprintable_down_arrow(void){
    ;
}

void __callback keyboard_nonprintable_right_arrow(void){
    input_cursor += ((input_cursor < (INPUT_BUFFER_CAP - 1)) && (input_buffer[input_cursor] != '\0'));
}

void __callback keyboard_nonprintable_left_arrow(void){
    input_cursor -= (input_cursor > 0);
}

void __callback keyboard_nonprintable_up_arrow(void){
    ;
}

void __callback keyboard_nonprintable_control(void){
    ;
}

void __callback keyboard_nonprintable_enter(void){
    sys_mode = SYSTEM_MODE_BUSY;
}

void __callback keyboard_nonprintable_backspace(void){
    if (!input_cursor) return;

    memmove(input_buffer + input_cursor - 1, input_buffer + input_cursor, INPUT_BUFFER_CAP - input_cursor - 1);
    input_cursor --;

    ros_cursor_copy(input_buffer + input_cursor, input_cursor, 1);
    ros_apply_output_entrys();
}

void __callback keyboard_nonprintable_tab(void){
    strcpy(input_buffer + input_cursor, "  ");
    input_cursor += 2;

    ros_cursor_copy(input_buffer + input_cursor - 2, input_cursor - 2, 0);
    ros_apply_output_entrys();
}