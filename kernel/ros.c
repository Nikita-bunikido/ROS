#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include <avr/io.h>
#include <avr/pgmspace.h>

#include "spi.h"
#include "st7735.h"

#define _INCLUDE_FONT
#include "font.h"
#include "ros.h"

#define OUTPUT_ENTRY_STACK_CAP      20

#define OUTPUT_ENTRY_PUSH(e)\
do {\
    if (output_entry_stack_size <= OUTPUT_ENTRY_STACK_CAP - 1){\
        output_entry_stack[output_entry_stack_size ++] = (e);\
        break;\
    }\
    apply_output_entrys();\
} while( 1 )

#define OUTPUT_ENTRY_POP(e)\
    (e) = output_entry_stack[--output_entry_stack_size]

static struct Output_Entry output_entry_stack[OUTPUT_ENTRY_STACK_CAP] = { 0 };
static int output_entry_stack_size = 0;
static volatile v2 cursor = { 0, 0 };

static inline __attribute__((always_inline)) uint16_t vga_to_rgb565(const uint8_t vga)
{
    uint16_t result = 0;
    if (vga & 0x1) result |= 0x1F << 11;
    if (vga & 0x2) result |= 0x3F << 5;
    if (vga & 0x4) result |= 0x1F << 0;
    return (result << 8) | (result >> 8); /* LE -> BE */
}

static void letter_lookup(uint8_t *dest, unsigned char let, uint8_t attrib, size_t dest_size)
{
    if (!dest)
        return;

    if (let > (sizeof(font) / 8))
        let = '?';

    for (unsigned row = 0; (row < LETTER_HEIGHT) && (dest_size > 0); ++row)
    for (unsigned col = 0; (col < LETTER_WIDTH) && (dest_size > 0); ++col, dest_size -= 2){
        *(uint16_t *)dest = vga_to_rgb565(attrib >> (((pgm_read_byte(&font[(int)let][row]) >> col) & 1) ? 0 : 4));
        dest += 2;
    }
}

static void apply_output_entrys(void)
{
    uint8_t letter_buffer[LETTER_WIDTH * LETTER_HEIGHT * 2];
    struct Output_Entry entry;
    
    while (output_entry_stack_size > 0){
        OUTPUT_ENTRY_POP(entry);
        v2 cur_pos = { entry.pos.x * LETTER_WIDTH, entry.pos.y * LETTER_HEIGHT };

        if (strchr("\n\r\v\t\a\f", entry.data) != NULL)
            continue;

        letter_lookup(letter_buffer, entry.data, struct_attribute_to_raw(entry.attrib), sizeof(letter_buffer));
        st7735_set_window(cur_pos.x, cur_pos.y, cur_pos.x + LETTER_WIDTH - 1, cur_pos.y + LETTER_HEIGHT - 1);
            
        BIT_ON(PORTB, 1); /* Start data stream */
        spi_device_transfer_buffer(letter_buffer, sizeof(letter_buffer));
        BIT_OFF(PORTB, 1); /* Stop data stream*/
    }
}

static v2 update_cursor(void) {
    if (cursor.x + 1 >= SCREEN_WIDTH / LETTER_WIDTH){
        cursor.x = 0;
        cursor.y ++;
        return cursor;
    }

    cursor.x ++;
    return cursor;
}

void ros_output_entry_push(struct Output_Entry oe) {
    OUTPUT_ENTRY_PUSH(oe);
}

void ros_apply_output_entrys(void) {
    apply_output_entrys();
}

void ros_putchar(uint8_t attrib, const char ch) {
    struct Output_Entry oe = (struct Output_Entry){ .pos = cursor, .attrib_raw = attrib, .data = (unsigned char)ch };
    OUTPUT_ENTRY_PUSH(oe);
    (void)update_cursor();
}

void ros_puts(uint8_t attrib, const char *str, bool new_line) {
    struct Output_Entry oe = (struct Output_Entry){ .pos = cursor, .attrib_raw = attrib };

    for(; *str; str++, oe.pos = update_cursor()){
        oe.data = *str;
        OUTPUT_ENTRY_PUSH(oe);
    }

    if (new_line) {
        cursor.x = 0;
        cursor.y ++;
        return;
    }
}

void ros_printf(uint8_t attrib, const char *format, ...) {
    va_list vptr;
    static char output_buffer[21];
    unsigned buffer_pos;

    va_start(vptr, format);

    if (strchr(format, '%') == NULL){
        /* No formats */
        ros_puts(attrib, format, format[strlen(format) - 1] == '\n');
        return;
    }

    for(buffer_pos = 0; *format && buffer_pos < (int)sizeof(output_buffer); format++){
        bool seq = true;

        switch (*format){
        case '\b':
            buffer_pos -= !!(buffer_pos > 0);
            break;

        case '\r':
            buffer_pos = 0;
            break;

        case '\t':
            output_buffer[buffer_pos] = output_buffer[buffer_pos + 1] = ' ';
            buffer_pos += 2;
            break;

        case '\n':
            output_buffer[buffer_pos] = '\0';
            ros_puts(attrib, output_buffer, true);

            buffer_pos = 0;
            break;
        
        default:
            seq = false;
            break;
        }

        if (seq)
            continue;
        
        if (*format != '%'){
            output_buffer[buffer_pos++] = *format;
            continue;
        }

        switch (*++format) {
        case 'd': case 'D':
            buffer_pos += snprintf(output_buffer + buffer_pos, sizeof(output_buffer) - buffer_pos, "%d", va_arg(vptr, int));
            break;
        
        case 's': case 'S':
            buffer_pos += snprintf(output_buffer + buffer_pos, sizeof(output_buffer) - buffer_pos, "%s", va_arg(vptr, char *));
            break;
        
        case 'f': case 'F':
        case 'g': case 'G':
            buffer_pos += snprintf(output_buffer + buffer_pos, sizeof(output_buffer) - buffer_pos, "%f", va_arg(vptr, double));
            break;

        case 'c': case 'C':
            output_buffer[buffer_pos++] = (char)va_arg(vptr, int);
            break;

        default:
            output_buffer[buffer_pos++] = *format;
        }
    }

    va_end(vptr);
    output_buffer[buffer_pos] = '\0';
    ros_puts(attrib, output_buffer, false);
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