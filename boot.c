#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>

#include <ros/ros-for-modules-all.h>

static const unsigned char preview[] PROGMEM = {
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x2e,0xa9,0xab,0xaa,0xaa,0xaa,0xab,0x20,0x20,0x20,0x20,0x20,0xa9,0xab,0xaa,0xaa,0xaa,0xab,0x20,0x20,
    0x20,0x81,0xaa,0xaa,0xae,0x20,0x7f,0xaa,0xaa,0x20,0x20,0x20,0x81,0xaa,0xaa,0xae,0x8c,0x20,0x20,0x20,0x20,
    0x20,0x80,0xaa,0xaa,0xab,0xab,0xab,0xaa,0xae,0x20,0x20,0x20,0x96,0xaa,0xaa,0xab,0xab,0xab,0x2c,0x20,0x20,
    0x20,0x7f,0xaa,0xaa,0xaa,0xab,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0xa2,0xae,0xae,0xae,0xaa,0xaa,0x20,0x20,
    0x20,0x9d,0xaa,0xaa,0x60,0xae,0x81,0xa5,0xab,0xaa,0xaa,0xaa,0xab,0x20,0x20,0x20,0x7f,0xaa,0xaa,0x20,0x20,
    0x20,0x97,0x81,0xaa,0x20,0x92,0x7f,0xaa,0xaa,0xae,0x20,0x7f,0xaa,0xaa,0x80,0xab,0xaa,0xaa,0xaa,0x20,0x20,
    0x20,0x8f,0x80,0xae,0x20,0x20,0x9b,0xaa,0xaa,0x20,0x20,0x29,0xaa,0xaa,0x9c,0xae,0xae,0xae,0x60,0x20,0x20,
    0x20,0x20,0x60,0x20,0x20,0x20,0x92,0xaa,0xaa,0xab,0x2c,0xa9,0xaa,0xaa,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x27,0x8f,0xae,0xaa,0xaa,0xaa,0xae,0x60,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x00
};

#define WELCOME_LEN     12u

void __callback welcome_flash(bool flag) {
    static struct Running_String_Info string_info = {
        .raw = USTR("Welcome to ROS! Type \'help\' to get started. | ROS (Rom Operating System) is a small, DOS-like, AVR-targetting operating system, written specially for my own computing machine NPAD-5 | It operates in text mode, without any UI, but applications can still draw TUI using pseudo graphics"),
        .attrib = { 1, 1, 0,  0,  1, 0, 1,  1 },
        .len = WELCOME_LEN,
        .offset = 0u
    };

    (void) flag;

    ros_puts_R(&string_info);
    string_info.offset = (string_info.offset + 1) % (strlen((const char *)(string_info.raw)) + 1);
}

static void test_ram(void) {
    for (int i = 0x55; i < 0xFF; i ++)
        memory_write(i, 0x55);
    
    for (int i = 0xFF - 1; i >= 0x55; i --)
        if (memory_read(i) != 0x55) HARD_ERROR(FAULT_RAM_MEMORY);
}

void ros_bootup(void) {
    /* from ros.c */
    extern void keyboard_input(enum Virtual_Key);

    /* Drivers */
    spi_device_init();
    st7735_init();

    /* Screen */
    clear_screen(0x0000);
    ros_puts_P(ATTRIBUTE_DEFAULT, preview, true);
    ros_flash(welcome_flash);
    ros_putchar(ATTRIBUTE_DEFAULT, '\n');
    keyboard_init(keyboard_input);
    ros_graphic_timer_init();

    /* Memory & keyboard */
    idle_key = INVALID_KEY;
    memory_init();
    test_ram();
    ros_log(LOG_TYPE_INFO, "RAM tested successfully");

    /* Cursor & prompt */
    ros_put_prompt();
    ros_apply_output_entrys();
    
    /* Enter input mode */
    sys_mode = SYSTEM_MODE_INPUT;
    enable_cursor();
    sei();
}

int main(void){
    ros_bootup();
    for(;;){}
}