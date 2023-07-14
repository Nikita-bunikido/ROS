CC = gcc
CFLAGS = -g0 -mno-ms-bitfields -s -O2 -Wall -Wextra -Wpedantic -pedantic -Wno-strict-aliasing -fno-common
CFLAGS += -Wno-int-to-pointer-cast -Wno-missing-braces -Wno-strict-aliasing -Wno-format
CFLAGS += -DNDEBUG
CFLAGS += -DANSI_SEQ
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