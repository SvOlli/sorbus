# Makefile for building the Sorbus test runner

CC = gcc
CFLAGS = -O3 -Wall
LDLIBS = -lm

# Target binary: RP2040 emulator (rp2350js-c.h) wired to the 65C02 CPU card
TARGET = test_runner
OBJS = test_runner.o

all: $(TARGET)

test_runner: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

test_runner.o: test_runner.c rp2350js-c.h m65c02.h

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
