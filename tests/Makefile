# Makefile for building the CPU card test library

CC = gcc
CFLAGS = -O2 -fPIC -shared -Wall
LDFLAGS = -shared

# Target library
TARGET = libcpu_card.so

# Source files
SOURCES = cpu_card_wrapper.c

# Build the shared library
$(TARGET): $(SOURCES) m65c02.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

# Clean build artifacts
clean:
	rm -f $(TARGET)

.PHONY: clean
