#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "driver.h"
#include "ros.h"

#define KEYBOARD_SHLD_PIN   0
#define KEYBOARD_CLK_PIN    1
#define KEYBOARD_SO_PIN     2

__driver void keyboard_init(void (*callback)(unsigned char));

#endif /* _KEYBOARD_H */