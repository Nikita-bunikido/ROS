# Common
DEVICE = atmega328p
F_CPU  = 16000000UL

# Compiler
CC = avr-gcc
CFLAGS = -DF_CPU=$(F_CPU) -Ofast -g -mmcu=$(DEVICE)
CFLAGS += -I include/
CFLAGS += -Wall -Wpedantic -Wextra 
CFLAGS += -Wno-array-bounds -Wno-format -Wno-pointer-arith

# Objects
DRIVERS_DIR = drivers
KERNEL_DIR = kernel

DRIVERS_OBJS = $(addsuffix .o, $(basename $(wildcard $(DRIVERS_DIR)/*.c) $(wildcard $(DRIVERS_DIR)/*.S)))
KERNEL_OBJS = $(addsuffix .o, $(basename $(wildcard $(KERNEL_DIR)/*.c) $(wildcard $(KERNEL_DIR)/*.S)))
OBJS = $(addsuffix .o, $(basename $(wildcard *.c))) $(DRIVERS_OBJS) $(KERNEL_OBJS)

default : main.hex

# Rules
%.o : %.S
	$(CC) -c $(CFLAGS) $< -o $@

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

main.hex : $(OBJS)
	$(CC) $(CFLAGS) $^ -o main.out
	avr-objcopy -O ihex -R .eeprom main.out main.hex

install : dummy.hex
	avrdude -v -V -P com4 -p ATMEGA328P -b 57600 -c arduino -U flash:w:$<

clean :
	del *.o
	del *.hex
	del *.bin
	del $(KERNEL_DIR)\*.o
	del $(DRIVERS_DIR)\*.o