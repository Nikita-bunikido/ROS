#ifndef _MEMORY_H
#define _MEMORY_H

#include <inttypes.h>
#include "ros.h"

#define MEMORY_CLK_PIN    4
#define MEMORY_DS_PIN     5
#define MEMORY_RW_PIN     0

typedef uint16_t Address;

void __driver memory_init(void);
void __driver memory_write(Address, uint8_t);
uint8_t __driver memory_read(Address);

#endif /* _MEMORY_H */