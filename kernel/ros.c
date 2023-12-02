#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <setjmp.h>

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/cpufunc.h>

#include <drivers/spi.h>
#include <drivers/memory.h>
#include <drivers/st7735.h>
#include <drivers/keyboard.h>

#include <ros/chip8.h>
#include <ros/video.h>
#include <ros/builtin.h>
#include <ros/log.h>
#include <ros/ros-for-modules.h>

#define MAX_ARGV        20

enum System_Mode sys_mode = SYSTEM_MODE_BUSY;

int caseless_cmp(const char *str1, const char *str2) {
    for(; *str1 && *str2 && (tolower(*str2) == tolower(*str1)); str1 ++, str2 ++);
    return tolower(*str1) - tolower(*str2);
}

void memory_cleanup(Address low, Address high) {
    for (Address i = low; i <= high; memory_write(i++, 0));
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

void __callback keyboard_nonprintable_down_arrow(void){ NOP(); }
void __callback keyboard_nonprintable_up_arrow(void){ NOP(); }
void __callback keyboard_nonprintable_control(void) { NOP(); }

void __callback keyboard_nonprintable_enter(void) {
    if (sys_mode == SYSTEM_MODE_BUSY)
        return;

    disable_cursor();
    sys_mode = SYSTEM_MODE_BUSY;
    ros_putchar(ATTRIBUTE_DEFAULT, '\n');

    size_t name_len;
    for (name_len = 0; !strchr(" \t", ibuffer.raw[name_len]); name_len ++);
    char *name = memcpy(__builtin_alloca(name_len), ibuffer.raw, name_len);
    name[name_len] = '\0';

    if (*name == '\0') {
        return_to_input_mode();
        return;
    }

    struct Builtin_Program program_info;
    if (get_builtin_program(&program_info, name) < 0) {
        ros_printf(ATTRIBUTE_DEFAULT, "Not found: \"%s\"\n", program_info.name);
        return_to_input_mode();
        return;
    }

    memory_write_buffer(0u, ibuffer.raw, sizeof ibuffer.raw);
    memory_write_buffer_P(PROGRAM_START, program_info.raw, program_info.size);

    struct Chip8_Context context;
    chip8_init(&context, PROGRAM_START);
    
    while (!context.halted && !setjmp(context.panic_buf))
        chip8_cycle(&context);

    if (context.exit_code)
        ros_printf(ATTRIBUTE_DEFAULT, "Program exited abnormally with code: %X\n", context.exit_code);

    #if defined(_SECURITY_KERNEL_CLEAR_PROGRAM)
        memory_cleanup(PROGRAM_START, PROGRAM_START + program_info.size);
    #endif
    
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