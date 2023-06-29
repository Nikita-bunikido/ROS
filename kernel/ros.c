#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include <util/delay.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "spi.h"
#include "st7735.h"

#include "video.h"
#include "keyboard.h"
#include "log.h"
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

struct Input_Buffer ibuffer = { 0 };

static void return_to_input_mode(void) {
    sys_mode = SYSTEM_MODE_INPUT;

    memset(ibuffer.raw, 0, INPUT_BUFFER_CAP);
    ibuffer.cursor = 0;

    ros_put_prompt();
    enable_cursor();
}

void __callback keyboard_input(enum Virtual_Key vk){
    int ch = vk_as_char(vk);

    if ((ch < 0) || (ibuffer.cursor >= INPUT_BUFFER_CAP - 1))
        return;

    memmove(ibuffer.raw + ibuffer.cursor + 1, ibuffer.raw + ibuffer.cursor, INPUT_BUFFER_CAP - ibuffer.cursor - 1);
    ibuffer.raw[ibuffer.cursor ++] = (char)ch;

    ros_put_input_buffer(ibuffer.cursor - 1, 0);
}

/* Keyboard callbacks are only active in SYSTEM_MODE_INPUT */
void __callback keyboard_nonprintable_right_arrow(void){
    ibuffer.cursor += ((ibuffer.cursor < (INPUT_BUFFER_CAP - 1)) && (ibuffer.raw[ibuffer.cursor] != '\0'));
    ros_put_input_buffer(ibuffer.cursor - 1, 0);
}

void __callback keyboard_nonprintable_left_arrow(void){
    ibuffer.cursor -= (ibuffer.cursor > 0);
    ros_put_input_buffer(ibuffer.cursor, 1);
}

void __callback keyboard_nonprintable_down_arrow(void){ __asm__ __volatile__ ("nop"); }
void __callback keyboard_nonprintable_up_arrow(void){ __asm__ __volatile__ ("nop"); }
void __callback keyboard_nonprintable_control(void) { __asm__ __volatile__ ("nop"); }

void __callback keyboard_nonprintable_enter(void){
    disable_cursor();
    sys_mode = SYSTEM_MODE_BUSY;
    ros_putchar(ATTRIBUTE_DEFAULT, '\n');

    for (int i = 0; i < 15; i++)
        ros_log(LOG_TYPE_INFO, "Test");
    
    return_to_input_mode();
}

void __callback keyboard_nonprintable_backspace(void){
    if (!ibuffer.cursor) return;

    memmove(ibuffer.raw + ibuffer.cursor - 1, ibuffer.raw + ibuffer.cursor, INPUT_BUFFER_CAP - ibuffer.cursor - 1);
    ibuffer.cursor --;

    ros_put_input_buffer(ibuffer.cursor, 2);
}

void __callback keyboard_nonprintable_tab(void){
    memmove(ibuffer.raw + ibuffer.cursor + 2, ibuffer.raw + ibuffer.cursor, INPUT_BUFFER_CAP - ibuffer.cursor - 2);
    memcpy(ibuffer.raw + ibuffer.cursor, "  ", 2);
    ibuffer.cursor += 2;

    ros_put_input_buffer(ibuffer.cursor - 2, 0);
}