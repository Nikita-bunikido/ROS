#ifndef _SPI_H
#define _SPI_H

#include <assert.h>
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

void __driver spi_device_init(void);
void __driver spi_device_deinit(void);

void __driver spi_device_transfer_byte(const uint8_t ch);
void __driver spi_device_transfer_buffer(const uint8_t *buffer, unsigned short buffer_size);

#endif /* _SPI_H */