CC = gcc
CFLAGS = -Wall -Werror -pthread -O3
LDFLAGS = -pthread
LDLIBS = -ldl -lwebpdemux -ljpeg -lpng -lgif

SOURCE = ./source
BUILD = ./build
TARGET = $(BUILD)/panelplayer
LIBRARY = $(BUILD)/libpanelplayer.so

# Installation directories
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include

HEADERS = $(wildcard $(SOURCE)/*.h)
MAIN_OBJECTS = $(patsubst $(SOURCE)/%.c,$(BUILD)/%.o,$(filter-out $(SOURCE)/panelplayer_api.c,$(wildcard $(SOURCE)/*.c)))
API_OBJECTS = $(patsubst $(SOURCE)/%.c,$(BUILD)/%.o,$(filter-out $(SOURCE)/main.c,$(wildcard $(SOURCE)/*.c)))

.PHONY: clean all library install uninstall

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

install: $(TARGET) $(LIBRARY)
	@echo "Installing PanelPlayer to $(PREFIX)..."
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/
	install -d $(DESTDIR)$(LIBDIR)
	install -m 644 $(LIBRARY) $(DESTDIR)$(LIBDIR)/
	install -d $(DESTDIR)$(INCLUDEDIR)/panelplayer
	install -m 644 $(SOURCE)/panelplayer_api.h $(DESTDIR)$(INCLUDEDIR)/panelplayer/
	@echo "Installation complete!"
	@echo ""
	@echo "Executable installed to: $(BINDIR)/panelplayer"
	@echo "Library installed to: $(LIBDIR)/libpanelplayer.so"
	@echo "Header installed to: $(INCLUDEDIR)/panelplayer/panelplayer_api.h"
	@echo ""
	@echo "You may need to run 'sudo ldconfig' to update the library cache."

uninstall:
	@echo "Uninstalling PanelPlayer from $(PREFIX)..."
	rm -f $(DESTDIR)$(BINDIR)/panelplayer
	rm -f $(DESTDIR)$(LIBDIR)/libpanelplayer.so
	rm -rf $(DESTDIR)$(INCLUDEDIR)/panelplayer
	@echo "Uninstall complete!"

clean:
	rm -r $(BUILD)