CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g
LDFLAGS = -pthread
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
TARGETS = supervisor generator
OBJS = $(BUILD_DIR)/circular_buffer.o $(BUILD_DIR)/utils.o

.PHONY: all clean

all: $(TARGETS)

# Supervisor target
supervisor: $(BUILD_DIR)/supervisor.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Generator target
generator: $(BUILD_DIR)/generator.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Object files for source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGETS)
