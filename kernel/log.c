#include <stdarg.h>

#include <avr/pgmspace.h>

#include "video.h"
#include "log.h"

#define ATTRIB_INFO    (struct Attribute){ 1, 1, 1,  0,  0, 0, 1,  1 }
#define ATTRIB_WARN    (struct Attribute){ 1, 1, 0,  0,  0, 0, 0,  1 }
#define ATTRIB_FAIL    (struct Attribute){ 1, 1, 1,  0,  1, 0, 0,  1 }

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

static void flash_puts(const char *data, uint8_t attrib, bool flag) {
    if (flag) VGA_SWITCH(attrib);
    ros_puts(attrib, USTR(data), false);
}

static void __callback flash_info_callback(bool flash) { flash_puts("INFO", struct_attribute_to_raw(ATTRIB_INFO), flash); }
static void __callback flash_warn_callback(bool flash) { flash_puts("WARN", struct_attribute_to_raw(ATTRIB_WARN), flash); }
static void __callback flash_fail_callback(bool flash) { flash_puts("FAIL", struct_attribute_to_raw(ATTRIB_FAIL), flash); }

static Flash_Routine flash_callbacks[LOG_TYPES_NUMBER - 1] = {
    [LOG_TYPE_INFO] = flash_info_callback,
    [LOG_TYPE_WARNING] = flash_warn_callback,
    [LOG_TYPE_ERROR] = flash_fail_callback,
};

static void __attribute__((noreturn)) enter_panic_mode(const int code) {
    sys_mode = SYSTEM_MODE_BUSY;
 
    clear_screen(0xf800);
    ros_printf(0x17, "**** STOP CODE: <%X>\n", code);
    ros_puts_P(0x17, panic_message, false);
    ros_puts_P(0x1F, panic_link, false);

    graphic_cursor = (struct Graphic_Cursor){
        .attrib_low = 0x17,
        .attrib_high = 0x1F,
        .visible = true
    };

    for(;;);
}

void ros_log(enum Log_Type type, const char *format, ...) {
    va_list vptr;
    va_start(vptr, format);

    if (type == LOG_TYPE_CRITICAL) {
        int code = va_arg(vptr, int);
        va_end(vptr);
        enter_panic_mode(code);
    }

    ros_flash(flash_callbacks[type]);
    ros_puts(ATTRIBUTE_DEFAULT, USTR("     "), false); /* 5 spaces ( log header + space ) */
    ros_vprintf(ATTRIBUTE_DEFAULT, format, vptr);  
    ros_putchar(ATTRIBUTE_DEFAULT, '\n');
}