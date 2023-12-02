# Common
DEVICE = atmega328p
F_CPU  = 16000000UL

# Compiler
CC = avr-gcc -mmcu=$(DEVICE)
CFLAGS = -pipe -pedantic -std=gnu11 -Os -g0 -fno-common -fno-exceptions -fno-pic -fno-pie
CFLAGS += -I include/ -I kernel/rch8/
CFLAGS += -Wall -Wpedantic -Wextra -Werror=attributes -Werror=implicit-function-declaration
CFLAGS += -Wno-array-bounds -Wno-format -Wno-pointer-arith -Wno-switch

# ROS Security
SECURITY_MAX = 1

ifeq ($(SECURITY_MAX), 1)
	CFLAGS += -D_SECURITY_API_ZERO_DIVIDER_CHECK
	CFLAGS += -D_SECURITY_KERNEL_CLEAR_PROGRAM
	CFLAGS += -D_SECURITY_KERNEL_STACK_PROTECTION
	CFLAGS += -D_SECURITY_KERNEL_ADVANCED_STACK_PROTECTION
endif

CFLAGS += -DF_CPU=$(F_CPU)

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