#include <stdbool.h>
#include <avr/interrupt.h>

#include "ros.h"
#include "spi.h"

#if SPI_SCK_FREQUENCY_PRESCALER == 4
    #define SPI_SPR (uint8_t)0
#elif SPI_SCK_FREQUENCY_PRESCALER == 16
    #define SPI_SPR (uint8_t)(BIT(SPR0))
#elif SPI_SCK_FREQUENCY_PRESCALER == 64
    #define SPI_SPR (uint8_t)(BIT(SPR1))
#elif SPI_SCK_FREQUENCY_PRESCALER == 128
    #define SPI_SPR (uint8_t)(BIT(SPR1) | BIT(SPR0))
#elif SPI_SCK_FREQUENCY_PRESCALER == 2
    #define SPI_SPR (uint8_t)(BIT(SPI2X))
#elif SPI_SCK_FREQUENCY_PRESCALER == 8
    #define SPI_SPR (uint8_t)(BIT(SPI2X) | BIT(SPR0))
#elif SPI_SCK_FREQUENCY_PRESCALER == 32
    #define SPI_SPR (uint8_t)(BIT(SPI2X) | BIT(SPR1))
#endif

volatile struct SPI_Device *SPI = (struct SPI_Device *)0x4C;

void __driver spi_device_init(void) {
    cli();
    spi_device_deinit();

    ROS_SET_PIN_DIRECTION(B, SPI_MOSI_PIN, PIN_DIRECTION_OUTPUT);
    ROS_SET_PIN_DIRECTION(B, SPI_SCK_PIN, PIN_DIRECTION_OUTPUT);
    ROS_SET_PIN_DIRECTION(B, SPI_SS_PIN, PIN_DIRECTION_OUTPUT);

    SPI->rSPCR = SPI_SPR;             /* MSB mode & SCK frequency */
    SPI->rSPCR |= BIT(MSTR);          /* Set master mode */
    SPI->rSPCR |= BIT(CPHA);          /* Leading Edge = Setup, Trailing Edge = Sample */
    SPI->rSPSR = 0;                   /* Status register */
    SPI->rSPCR |= BIT(SPE);           /* Enable SPI */
    sei();
}

void __driver spi_device_deinit(void) {
    BIT_OFF(SPI->rSPCR, SPE);
}

void __driver spi_device_transfer_byte(const uint8_t ch) {
    SPI->rSPDR = ch;
    
    while (!(SPI->rSPSR & BIT(SPIF)))
        ;
}

void __driver spi_device_transfer_buffer(const uint8_t *buffer, unsigned short buffer_size) {
    if (!buffer_size)
        return;

    while (buffer_size-- > 0)
        spi_device_transfer_byte(*buffer++);
}