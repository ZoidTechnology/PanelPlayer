CC = gcc
CFLAGS = -Wall -Werror -pthread -O3
LDFLAGS = -pthread
LDLIBS = -ldl -lwebpdemux

SOURCE = ./source
BUILD = ./build
TARGET = $(BUILD)/panelplayer

HEADERS = $(wildcard $(SOURCE)/*.h)
OBJECTS = $(patsubst $(SOURCE)/%.c,$(BUILD)/%.o,$(wildcard $(SOURCE)/*.c))

.PHONY: clean

$(TARGET): $(BUILD) $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

$(BUILD):
	mkdir $(BUILD)

$(BUILD)/%.o: $(SOURCE)/%.c $(HEADERS) makefile
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -r $(BUILD)