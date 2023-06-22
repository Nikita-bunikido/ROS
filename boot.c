#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdarg.h>

#include <util/delay.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "spi.h"
#include "st7735.h"
#include "keyboard.h"

#include "font.h"
#include "video.h"
#include "ros.h"

void keyboard_callback(enum Virtual_Key vk){
    int ch = vk_as_char(vk);

    if (ch > 0)
        ros_putchar(0xF, ch);

#ifndef NDEBUG
    ros_apply_output_entrys();
#endif
}

int main(void){
    SPI = spi_get();
    spi_device_init();
    st7735_init();
    keyboard_init(keyboard_callback);

    struct Attribute sa = (struct Attribute){
        .blink = 0,
        .back_b = 1,
        .back_g = 0,
        .back_r = 0,
        .reserved = 0,
        .fore_b = 1,
        .fore_g = 1,
        .fore_r = 1
    };
    ros_printf(struct_attribute_to_raw(sa), "Welcome to ROS!\n");
    ros_apply_output_entrys();

    sei();
    for(;;){}
}