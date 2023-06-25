#include <stdarg.h>

#include <avr/pgmspace.h>

#include "video.h"
#include "log.h"

static const struct Attribute prefix_colors[LOG_TYPES_NUMBER - 1] PROGMEM = {
    [LOG_TYPE_INFO] = { 1, 1, 1,  0,  0, 0, 1,  1 },
    [LOG_TYPE_ERROR] = { 1, 1, 1,  0,  1, 0, 0,  1 },
    [LOG_TYPE_WARNING] = { 0, 0, 0,  0,  1, 1, 0,  1 },
};

static const unsigned char panic_message[] PROGMEM = 
    ":_(\n\n"
    "It seems that something went wrong, and ROS can no longer work safely.\n"
    "What you can do:\n"
    "- Restart your machine, and see if something has changed\n"
    "- Check your hardware, that could cause the problem\n"
    "- Reinstall your system completely\n"
    "- Update your system. For more :"
, panic_link[] PROGMEM = 
    "https://github.com/Nikita-bunikido/ROS";

static void __attribute__((noreturn)) enter_panic_mode(const int code) {
    sys_mode = SYSTEM_MODE_IDLE;
    clear_screen(0xf800);
    ros_printf(0x17, "**** STOP CODE: <%X>\n", code);
    ros_puts_P(0x17, panic_message, false);
    ros_puts_P(0x1F, panic_link, true);
    ros_apply_output_entrys();

    for(;;);
}

void ros_log(enum Log_Type type, const char *format, ...) {
    const char *type_cstr = LOG_TYPE_CSTR [type];
    va_list vptr;

    va_start(vptr, format);

    if (type == LOG_TYPE_CRITICAL) {
        va_end(vptr);
        enter_panic_mode(va_arg(vptr, int));
    }
 
    ros_puts(pgm_read_byte(&prefix_colors[type]), USTR(type_cstr), false);
    ros_putchar(ATTRIBUTE_DEFAULT, ' ');
    ros_vprintf(ATTRIBUTE_DEFAULT, format, vptr);  
}