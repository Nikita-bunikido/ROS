#ifndef _SPI_H
#define _SPI_H

#include <assert.h>
#include <avr/io.h>

#include "driver.h"
#include "ros.h"

#define SPI_SCK_FREQUENCY_PRESCALER     4

#define SPI_SS_PIN      2
#define SPI_MOSI_PIN    3
#define SPI_SCK_PIN     5

struct PACKED SPI_Device {
    volatile uint8_t rSPCR;
    volatile uint8_t rSPSR;
    volatile uint8_t rSPDR;
};
static_assert( sizeof(struct SPI_Device) == 3 );

extern volatile struct SPI_Device *SPI;

static inline __attribute__((always_inline)) struct SPI_Device *spi_get(void){
    return (struct SPI_Device *)0x4C;
}

__driver void spi_device_init(void);
__driver void spi_device_deinit(void);

__driver void spi_device_transfer_byte(const uint8_t ch);
__driver void spi_device_transfer_buffer(const uint8_t *buffer, size_t buffer_size);

#endif /* _SPI_H */