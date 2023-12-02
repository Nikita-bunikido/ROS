#ifndef _MEMORY_H
#define _MEMORY_H

#include <inttypes.h>

#include <ros/ros-for-headers.h>

enum {
    MEMORY_CLK_PIN = 4,
    MEMORY_DS_PIN  = 5,
    MEMORY_RW_PIN  = 0,
};

typedef uint16_t Address;

void __driver memory_init(void);
uint8_t __driver memory_write(Address, uint8_t);
uint8_t __driver memory_read(Address);
void memory_write_buffer(Address, const void *, size_t);
void memory_write_buffer_P(Address, const void *, size_t);

inline ALWAYS_INLINE uint16_t memory_read_word(Address addr) {
    return (uint16_t)memory_read(addr) | (uint16_t)memory_read(addr + 1) << 8;
}

inline ALWAYS_INLINE void memory_write_word(Address addr, uint16_t word) {
    memory_write(addr, word & 0xFF);
    memory_write(++addr, word >> 8);
}

#endif /* _MEMORY_H */