CC = gcc
CFLAGS = -Wall -Werror -pthread -O3
LDFLAGS = -pthread
LDLIBS = -ldl -lwebpdemux -ljpeg -lpng -lgif

SOURCE = ./source
BUILD = ./build
TARGET = $(BUILD)/panelplayer
LIBRARY = $(BUILD)/libpanelplayer.so

HEADERS = $(wildcard $(SOURCE)/*.h)
MAIN_OBJECTS = $(patsubst $(SOURCE)/%.c,$(BUILD)/%.o,$(filter-out $(SOURCE)/panelplayer_api.c,$(wildcard $(SOURCE)/*.c)))
API_OBJECTS = $(patsubst $(SOURCE)/%.c,$(BUILD)/%.o,$(filter-out $(SOURCE)/main.c,$(wildcard $(SOURCE)/*.c)))

.PHONY: clean all library

all: $(TARGET) $(LIBRARY)

$(TARGET): $(BUILD) $(MAIN_OBJECTS)
	$(CC) $(LDFLAGS) $(MAIN_OBJECTS) $(LDLIBS) -o $@

$(LIBRARY): $(BUILD) $(API_OBJECTS)
	$(CC) $(LDFLAGS) -shared -fPIC $(API_OBJECTS) $(LDLIBS) -o $@

library: $(LIBRARY)

$(BUILD):
	mkdir $(BUILD)

$(BUILD)/%.o: $(SOURCE)/%.c $(HEADERS) makefile
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

clean:
	rm -r $(BUILD)