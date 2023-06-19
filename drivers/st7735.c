#include <stdbool.h>

#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "ros.h"
#include "spi.h"
#include "st7735.h"

static volatile bool freezed = false;

static void st7735_send_command(const ST7735_Command command){
    BIT_OFF(PORTB, 1);
    BIT_OFF(PORTB, SPI_SS_PIN);

    spi_device_transfer_byte(command.type);
    
    if (command.nargs > 0){
        BIT_ON(PORTB, 1);
        spi_device_transfer_buffer(command.args, command.nargs);
        BIT_OFF(PORTB, 1);
    }

    if (!command.delay_after) return;
    int delay = command.delay_after == 0xFF ? 500 : (int)command.delay_after;

    for (int i = 0; i < delay; ++i)
        _delay_ms(1);
}

static const struct ST7735_Command startup[] PROGMEM = {
    /* RCMD1 */
    { ST7735_SWRESET, 0, { 0 }, 150 },              /* Software reset | 120 ms wait  */
    { ST7735_SLPOUT, 0, { 0 }, 255 },               /* Sleep mode : OFF | 120 ms wait */
    { ST7735_FRMCTR1, 3, { 0x01, 0x2C, 0x2D }, 0 },
    { ST7735_FRMCTR2, 3, { 0x01, 0x2C, 0x2D }, 0 },
    { ST7735_FRMCTR3, 6, { 0x01, 0x2C, 0x2D,  0x01, 0x2C, 0x2D }, 0 },
    { ST7735_INVCTR, 1, { 0x07 }, 0 },
    { ST7735_PWCTR1, 3, { 0xA2, 0x02, 0x84 }, 0 },
    { ST7735_PWCTR2, 1, { 0xC5 }, 0 },
    { ST7735_PWCTR3, 2, { 0x0A, 0x00 }, 0 },
    { ST7735_PWCTR4, 2, { 0x8A, 0x2A }, 0 },
    { ST7735_PWCTR5, 2, { 0x8A, 0xEE }, 0 },
    { ST7735_VMCTR1, 1, { 0x0E }, 0 },
    { ST7735_INVOFF, 0, { 0 }, 0 },                 /* Inversion OFF */
    { ST7735_COLMOD, 1, { 0x05 }, 0 },              /* Color mode RGB16 */

    /* RCMD3 */
    { ST7735_GMCTRP1, 16, { 0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10 }, 0 },
    { ST7735_GMCTRN1, 16, { 0x03, 0x1d, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10 }, 0 },
    { ST7735_NORON, 0, { 0 }, 10 },
    { ST7735_DISPON, 0, { 0 }, 100 },

    { ST7735_MADCTL, 1, { 0 }, 0 },              /* Bottom to top refresh */

    /* Rectangle */
    { ST7735_CASET, 4, { 0, 0, 0, SCREEN_WIDTH }, 0 },
    { ST7735_RASET, 4, { 0, 0, 0, SCREEN_HEIGHT }, 0 },
    { ST7735_RAMWR, 0, { 0 }, 150 }
};

static ST7735_Command pgm_read_st7735_command(const void *mem)
{
    ST7735_Command com;
    memcpy_P(&com, mem, sizeof(com));
    return com;
}

void st7735_init(void){
    cli();

    ROS_SET_PIN_DIRECTION(B, 1, PIN_DIRECTION_OUTPUT);

    /* Startup */
    for (unsigned i = 0; i < sizeof(startup) / sizeof(startup[0]); ++i)
        st7735_send_command(pgm_read_st7735_command(&startup[i]));
    
    BIT_OFF(PORTB, SPI_SS_PIN);
    BIT_ON(PORTB, 1);
}

void st7735_set_window(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2){
    if (x1 > SCREEN_WIDTH + 1) x1 = SCREEN_WIDTH + 1;
    if (y1 > SCREEN_HEIGHT + 1) y1 = SCREEN_HEIGHT + 1;
    if (x2 > SCREEN_WIDTH + 1) x2 = SCREEN_WIDTH + 1;
    if (y2 > SCREEN_HEIGHT + 1) y2 = SCREEN_HEIGHT + 1;

    st7735_send_command((struct ST7735_Command){ ST7735_CASET, 4, { 0, x1, 0, x2 }, 0 });
    st7735_send_command((struct ST7735_Command){ ST7735_RASET, 4, { 0, y1, 0, y2 }, 0 });
    st7735_send_command((struct ST7735_Command){ ST7735_RAMWR, 0, { 0 }, 0 });
}

void st7735_deinit(void){
    ;
}

void st7735_freeze(void){
    BIT_ON(PORTB, SPI_SS_PIN);
}

void st7735_unfreeze(void){
    BIT_OFF(PORTB, SPI_SS_PIN);
}