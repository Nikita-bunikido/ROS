#ifndef _ST7735_H
#define _ST7735_H

#include <avr/io.h>

#include "ros.h"

enum ST7735_Command_Type {
    ST7735_NOP       = 0x00,
    ST7735_SWRESET   = 0x01,
    ST7735_RDDID     = 0x04,
    ST7735_RDDST     = 0x09,
    ST7735_RDDPM     = 0x0A,
    ST7735_RDDMADCTL = 0x0B,
    ST7735_RDDCOLMOD = 0x0C,
    ST7735_RDDIM     = 0x0D,
    ST7735_RDDSM     = 0x0E,
    ST7735_SLPIN     = 0x10,
    ST7735_SLPOUT    = 0x11,
    ST7735_PTLON     = 0x12,
    ST7735_NORON     = 0x13,
    ST7735_INVOFF    = 0x20,
    ST7735_INVON     = 0x21,
    ST7735_GAMSET    = 0x26,
    ST7735_DISPOFF   = 0x28,
    ST7735_DISPON    = 0x29,
    ST7735_CASET     = 0x2A,
    ST7735_RASET     = 0x2B,
    ST7735_RAMWR     = 0x2C,
    ST7735_RAMRD     = 0x2E,
    ST7735_PTLAR     = 0x30,
    ST7735_TEOFF     = 0x34,
    ST7735_TEON      = 0x35,
    ST7735_MADCTL    = 0x36,
    ST7735_IDMOFF    = 0x38,
    ST7735_IDMON     = 0x39,
    ST7735_COLMOD    = 0x3A,
    ST7735_RDID1     = 0xDA,
    ST7735_RDID2     = 0xDB,
    ST7735_RDID3     = 0xDC,

    ST7735_FRMCTR1   = 0xB1,
    ST7735_FRMCTR2   = 0xB2,
    ST7735_FRMCTR3   = 0xB3,
    ST7735_INVCTR    = 0xB4,
    ST7735_DISSET5   = 0xB6,
    ST7735_PWCTR1    = 0xC0,
    ST7735_PWCTR2    = 0xC1,
    ST7735_PWCTR3    = 0xC2,
    ST7735_PWCTR4    = 0xC3,
    ST7735_PWCTR5    = 0xC4,
    ST7735_VMCTR1    = 0xC5,
    ST7735_VMOFCTR   = 0xC7,
    ST7735_WRID2     = 0xD1,
    ST7735_WRID3     = 0xD2,
    ST7735_PWCTR6    = 0xFC,
    ST7735_NVFCTR1   = 0xD9,
    ST7735_NVFCTR2   = 0xDE,
    ST7735_NVFCTR3   = 0xDF,
    ST7735_GMCTRP1   = 0xE0,
    ST7735_GMCTRN1   = 0xE1,
    ST7735_EXTCTRL   = 0xF0,
    ST7735_VCOM4L    = 0xFF,
};

typedef struct ST7735_Command {
    uint8_t type;
    int nargs;

    uint8_t args[24];
    uint8_t delay_after;
} ST7735_Command;

__driver void st7735_init(void);
__driver void st7735_set_window(uint8_t, uint8_t, uint8_t, uint8_t);
__driver void st7735_deinit(void);
__driver void st7735_freeze(void);
__driver void st7735_unfreeze(void);

#endif /* _ST7735_H */