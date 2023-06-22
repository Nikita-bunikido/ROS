#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include <avr/io.h>
#include <avr/pgmspace.h>

#include "spi.h"
#include "st7735.h"

#include "ros.h"

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