#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "ros.h"

#define KEYBOARD_SHLD_PIN       0
#define KEYBOARD_CLK_PIN        1
#define KEYBOARD_SO_PIN         2
#define KEYBOARD_INTERRUPT_PIN  2

#define KEY(sym,code)   VK_##sym = code,
enum Virtual_Key {
    #include "keyboard.def"
};
#undef KEY

typedef void (*__callback Keyboard_Nonprintable_Callback)(void);
typedef void (*__callback Keyboard_User_Callback)(enum Virtual_Key);

int vk_as_char(enum Virtual_Key key);
void __driver keyboard_init(Keyboard_User_Callback, Keyboard_User_Callback);

#endif /* _KEYBOARD_H */