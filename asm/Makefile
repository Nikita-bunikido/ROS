CC = gcc
CFLAGS = -g0 -s -O2 -Wall -Wextra -Wpedantic -Wno-strict-aliasing -fno-common
TARGET = asm.exe

OBJS = $(addsuffix .o, $(basename $(wildcard *.c) $(wildcard *.S)))

default : $(TARGET)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.exe : $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	del *.o
	del *.exe