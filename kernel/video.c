#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include <avr/io.h>
#include <avr/pgmspace.h>

#include "spi.h"
#include "st7735.h"

#define _INCLUDE_FONT
#include "font.h"
#include "video.h"
#include "ros.h"

#define OUTPUT_ENTRY_STACK_CAP      20

#define OUTPUT_ENTRY_PUSH(e)                                    \
do {                                                            \
    if (output_entry_stack_size <= OUTPUT_ENTRY_STACK_CAP - 1){ \
        output_entry_stack[output_entry_stack_size ++] = (e);   \
        break;                                                  \
    }                                                           \
    apply_output_entrys();                                      \
} while( 1 )                                                    \

#define OUTPUT_ENTRY_POP(e)                                     \
    (e) = output_entry_stack[--output_entry_stack_size]         \

static volatile v2 cursor = { 0, 0 };

static struct Output_Entry output_entry_stack[OUTPUT_ENTRY_STACK_CAP] = { 0 };
static unsigned short output_entry_stack_size = 0;

static inline __attribute__((always_inline, const)) uint16_t vga_to_rgb565(const uint8_t raw) {
    uint16_t result = 0;
    if (raw & 0x1) result |= 0x1F << 11;
    if (raw & 0x2) result |= 0x3F << 5;
    if (raw & 0x4) result |= 0x1F << 0;
    return (result << 8) | (result >> 8); /* LE -> BE */
}

static void letter_lookup(uint8_t *dest, unsigned char let, uint8_t attrib, size_t dest_size) {
    if (!dest)
        return;

    if (let > (sizeof(font) / 8) - 1)
        let = (sizeof(font) / 8) - 1;

    for (unsigned row = 0; (row < LETTER_HEIGHT) && (dest_size > 0); ++row)
    for (unsigned col = 0; (col < LETTER_WIDTH) && (dest_size > 0); ++col, dest_size -= 2){
        
        bool underline = (!!BIT_EXT(attrib, 3)) && (row == LETTER_HEIGHT - 1);
        *(uint16_t *)dest = vga_to_rgb565(attrib >> ((((pgm_read_byte(&font[(int)let][row]) >> col) & 1) || underline) ? 0 : 4));
        dest += 2;
    }
}

static void apply_output_entrys(void) {
    static uint8_t letter_buffer[LETTER_WIDTH * LETTER_HEIGHT * 2];
    struct Output_Entry entry;
    
    while (output_entry_stack_size > 0){
        OUTPUT_ENTRY_POP(entry);
        v2 cur_pos = { entry.pos.x * LETTER_WIDTH, entry.pos.y * LETTER_HEIGHT };

        if ((strchr("\n\r\v\t\a\f", entry.data) != NULL) || (entry.data < ' '))
            continue;

        letter_lookup(letter_buffer, entry.data, struct_attribute_to_raw(entry.attrib), sizeof(letter_buffer));
        st7735_set_window(cur_pos.x, cur_pos.y, cur_pos.x + LETTER_WIDTH - 1, cur_pos.y + LETTER_HEIGHT - 1);
            
        BIT_ON(PORTB, ST7735_DC_PIN);
        spi_device_transfer_buffer(letter_buffer, sizeof(letter_buffer));
        BIT_OFF(PORTB, ST7735_DC_PIN);
    }
}

static v2 move_cursor_forward(void) {
    if (cursor.x + 1 >= SCREEN_WIDTH / LETTER_WIDTH){
        cursor.x = 0;
        cursor.y ++;
        return cursor;
    }

    cursor.x ++;
    return cursor;
}

unsigned char ros_putchar(uint8_t attrib, const unsigned char ch) {
    OUTPUT_ENTRY_PUSH((struct Output_Entry){ .pos = cursor COMMA .attrib_raw = attrib COMMA .data = ch });
    move_cursor_forward();
    return ch;
}

int ros_puts(uint8_t attrib, const unsigned char *str, bool new_line) {
    struct Output_Entry oe = (struct Output_Entry){ .pos = cursor, .attrib_raw = attrib };
    int printed;

    for(printed = 0; *str; str++, oe.pos = move_cursor_forward()){
        oe.data = *str;
        printed ++;
        OUTPUT_ENTRY_PUSH(oe);
    }

    if (!new_line)
        return printed;

    cursor.x = 0;
    cursor.y ++;
    return ++printed;
}

int ros_puts_P(uint8_t attrib, const unsigned char *str, bool new_line) {
    struct Output_Entry oe = (struct Output_Entry){ .pos = cursor, .attrib_raw = attrib };
    int printed = 0;
    char ch;

    while ((ch = pgm_read_byte(str)) != '\0') {
        if (ch == '\n') {
            cursor.x = 0;
            cursor.y ++;
            printed ++;
            str ++;
            oe.pos = cursor;
            continue;
        }

        oe.data = ch;
        OUTPUT_ENTRY_PUSH(oe);

        oe.pos = move_cursor_forward();
        str ++;
        printed ++;
    }

    if (!new_line)
        return printed;

    cursor.x = 0;
    cursor.y ++;
    return ++printed;
}

int ros_vprintf(uint8_t attrib, const char *format, va_list vptr) {
    static char output_buffer[21];
    int printed;
    unsigned short buffer_pos;

    /* No formats */
    if (strchr(format, '%') == NULL)
        return ros_puts(attrib, USTR(format), format[strlen(format) - 1] == '\n');

    for (buffer_pos = 0, printed = 0; *format && buffer_pos < sizeof(output_buffer); format++){
        bool seq = true;

        switch (*format) {
        case '\b':
            buffer_pos -= !!(buffer_pos > 0);
            break;

        case '\r':
            buffer_pos = 0;
            break;

        case '\t':
            output_buffer[buffer_pos] = output_buffer[buffer_pos + 1] = ' ';
            buffer_pos += 2;
            printed += 2;
            break;

        case '\n':
            output_buffer[buffer_pos] = '\0';
            printed += ros_puts(attrib, USTR(output_buffer), true);

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
            printed ++;
            continue;
        }

        unsigned short plen = 0;
        switch (*++format) {
        case 'd': case 'D':
            plen = snprintf(output_buffer + buffer_pos, sizeof(output_buffer) - buffer_pos, "%d", va_arg(vptr, int));
            break;

        case 'x': case 'X':
            plen = snprintf(output_buffer + buffer_pos, sizeof(output_buffer) - buffer_pos, (*format == 'X') ? "%X" : "%x", va_arg(vptr, int));
            break;

        case 's': case 'S':
            plen = snprintf(output_buffer + buffer_pos, sizeof(output_buffer) - buffer_pos, "%s", va_arg(vptr, char *));
            break;
        
        case 'f': case 'F':
        case 'g': case 'G':
            plen = snprintf(output_buffer + buffer_pos, sizeof(output_buffer) - buffer_pos, "%f", va_arg(vptr, double));
            break;

        case 'c': case 'C':
            output_buffer[buffer_pos++] = (char)va_arg(vptr, int);
            break;

        default:
            output_buffer[buffer_pos++] = *format;
        }

        buffer_pos += plen;
        printed += plen;
    }

    va_end(vptr);
    output_buffer[buffer_pos] = '\0';
    printed += ros_puts(attrib, USTR(output_buffer), false);
    return printed;
}

int ros_printf(uint8_t attrib, const char *format, ...) {
    va_list vptr;
    va_start(vptr, format);

    return ros_vprintf(attrib, format, vptr);
}

void ros_apply_output_entrys(void) {
    #ifndef NDEBUG
        apply_output_entrys();
    #endif
}

/* from ros.c */
extern struct Input_Buffer ibuffer;

void ros_put_input_buffer(unsigned short disp, int overlap) {
    const v2 old_cursor = cursor;
    const unsigned short odisp = disp;

    while (disp --)
        move_cursor_forward();
    
    ros_puts(ATTRIBUTE_DEFAULT, USTR(ibuffer.raw + odisp), false);

    if (overlap > 0)
        while (overlap --)
            ros_putchar(ATTRIBUTE_DEFAULT, 0x20);

    cursor = old_cursor;
}

void ros_put_graphic_cursor(void) {
    v2 target = cursor;
    
    if (sys_mode != SYSTEM_MODE_BUSY)
        target = (v2){ ( target.x + ibuffer.cursor ) % (SCREEN_WIDTH / LETTER_WIDTH), ( target.y + ibuffer.cursor / (SCREEN_WIDTH / LETTER_WIDTH)) };

    v2 old_cursor = cursor;
    cursor = target;
    ros_putchar(ATTRIBUTE_UNDERLINE, (sys_mode == SYSTEM_MODE_INPUT) ? (ibuffer.raw[ibuffer.cursor] ? ibuffer.raw[ibuffer.cursor] : ' ') : ' ');
    
    cursor = old_cursor;
}

void ros_put_prompt(void) { ros_puts(ATTRIBUTE_DEFAULT, USTR("$ "), false); }

void clear_screen(uint16_t rgb565) {
    cursor = (v2){ 0, 0 };
    st7735_set_window(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    BIT_ON(PORTB, ST7735_DC_PIN);
    for (unsigned y = 0; y < SCREEN_HEIGHT + 1; y++)
    for (unsigned x = 0; x < SCREEN_WIDTH + 1; x++){
        spi_device_transfer_byte(HI8(rgb565));
        spi_device_transfer_byte(LO8(rgb565));
    }
    BIT_OFF(PORTB, ST7735_DC_PIN);
}