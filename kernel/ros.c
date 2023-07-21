#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <setjmp.h>

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "spi.h"
#include "memory.h"
#include "st7735.h"

#include "chip8.h"
#include "video.h"
#include "keyboard.h"
#include "builtin.h"
#include "log.h"
#include "ros.h"

enum System_Mode sys_mode = SYSTEM_MODE_BUSY;

static inline __attribute__((always_inline)) int mtolower(int ch) {
    return (ch >= 'A') && (ch <= 'Z') ? tolower(ch) : ch;
}

int caseless_cmp(const char *str1, const char *str2) {
    for(; *str1 && *str2 && (mtolower(*str2) == mtolower(*str1)); str1 ++, str2 ++);
    return mtolower(*str1) - mtolower(*str2);
}

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

static int load_rex(void) {
    struct Chip8_Context context;
    uint16_t size;
    uint8_t *raw;

    if ((raw = get_builtin_program(&ibuffer, &size)) == NULL)
        return -1;

    for (Address addr = PROGRAM_START; addr < (Address)(size + PROGRAM_START); addr++, raw++)
        memory_write(addr, pgm_read_byte(raw));

    chip8_init(&context);
    while (!context.halted && !setjmp(chip8_panic_buf))
        chip8_cycle(&context);

    if (!context.exit_code)
        ros_log(LOG_TYPE_INFO, "Kernel level program executed successfully.");
    else {
        ros_log(LOG_TYPE_ERROR, "Kernel level program failed.");
        ros_printf(ATTRIBUTE_DEFAULT, "Exit code: %X", context.exit_code);
    }

    ros_putchar(ATTRIBUTE_DEFAULT, '\n');
    return 0;
}

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

    if (load_rex() < 0)
        ros_log(LOG_TYPE_ERROR, "Could not find specified program.");

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

ISR(BADISR_vect) {
    /* Unknown interrupt */
    HARD_ERROR(FAULT_KERNEL_BAD_INTERRUPT);
}