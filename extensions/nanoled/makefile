CC = gcc
CFLAGS = -Wall -Werror -fPIC -O3
LDFLAGS = -shared

SOURCE = ./source
BUILD = ./build
TARGET = $(BUILD)/extension.so

HEADERS = $(wildcard $(SOURCE)/*.h)
OBJECTS = $(patsubst $(SOURCE)/%.c,$(BUILD)/%.o,$(wildcard $(SOURCE)/*.c))

.PHONY: clean

$(TARGET): $(BUILD) $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

$(BUILD):
	mkdir $(BUILD)

$(BUILD)/%.o: $(SOURCE)/%.c $(HEADERS) makefile
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -r $(BUILD)