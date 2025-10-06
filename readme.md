# PanelPlayer

A high-performance media player for Colorlight LED receiving cards, supporting multiple image and animation formats. Tested with the Colorlight 5A-75B.

## Features

- ✅ **Multi-format Support**: WebP, JPEG, PNG, GIF, and BMP
- ✅ **Hardware Acceleration**: Direct ethernet control via raw sockets
- ✅ **Extension System**: Custom frame processing plugins
- ✅ **C API**: Full-featured library for integration
- ✅ **Cross-language Bindings**: C# wrapper included
- ✅ **Frame Mixing**: Smooth transitions between frames
- ✅ **Duplicate Mode**: Drive stacked displays easily

## Quick Start

```bash
# Install dependencies
sudo apt install libwebp-dev libjpeg-dev libpng-dev libgif-dev

# Build and install
make
sudo make install
sudo ldconfig

# Run
panelplayer -p eth0 -w 128 -h 64 animation.webp
```

## Usage

### Command Line

```bash
panelplayer [options] <sources>
```

#### Required Options

| Option | Description | Example |
|--------|-------------|---------|
| `-p <port>` | Ethernet interface | `-p eth0` |
| `-w <width>` | Display width in pixels | `-w 128` |
| `-h <height>` | Display height in pixels | `-h 64` |

#### Optional Settings

| Option | Description | Default |
|--------|-------------|---------|
| `-b <brightness>` | Brightness (0-255) | 255 |
| `-m <mix>` | Frame mixing percentage (0-99) | 0 |
| `-r <rate>` | Override frame rate (fps) | Auto |
| `-e <path>` | Load extension | None |
| `-d` | Duplicate mode (for stacked displays) | Disabled |
| `-s` | Shuffle/loop playback | Disabled |
| `-v` | Verbose output | Disabled |

### Examples

**Play a single image:**
```bash
panelplayer -p eth0 -w 192 -h 64 photo.jpg
```

**Play animated WebP with verbose output:**
```bash
panelplayer -p eth0 -w 128 -h 64 -v animation.webp
```

**Play multiple files with frame blending:**
```bash
panelplayer -p eth0 -w 192 -h 64 -m 50 video1.webp video2.gif video3.png
```

**Loop with shuffle mode:**
```bash
panelplayer -p eth0 -w 128 -h 64 -s *.webp
```

**Use extension for grayscale effect:**
```bash
panelplayer -p eth0 -w 128 -h 64 -e extensions/grayscale/extension.so image.jpg
```

**Duplicate mode for two stacked 128x64 panels (total 128x128):**
```bash
panelplayer -p eth0 -w 128 -h 64 -d content.png
```

## Building

### Prerequisites

Install development libraries:
```bash
# Debian/Ubuntu
sudo apt install libwebp-dev libjpeg-dev libpng-dev libgif-dev

# Fedora/RHEL
sudo dnf install libwebp-devel libjpeg-devel libpng-devel giflib-devel
```

### Compile

```bash
make              # Build both executable and library
make library      # Build only the shared library
make clean        # Clean build artifacts
```

### Installation

Install system-wide (requires root):
```bash
sudo make install
sudo ldconfig
```

This installs:
- **Executable**: `/usr/local/bin/panelplayer`
- **Library**: `/usr/local/lib/libpanelplayer.so`
- **Header**: `/usr/local/include/panelplayer/panelplayer_api.h`

Uninstall:
```bash
sudo make uninstall
```

Custom installation prefix:
```bash
make install PREFIX=/opt/panelplayer
```

## Library Usage

### C API

The library provides a stateful API for programmatic control:

```c
#include <panelplayer/panelplayer_api.h>

int main() {
    // Initialize
    panelplayer_init("eth0", 128, 64, 255);

    // Configure
    panelplayer_set_mix(30);
    panelplayer_set_rate(30);

    // Play content
    panelplayer_play_file("animation.webp");

    // Or send raw frames
    uint8_t frame[128 * 64 * 3];  // BGR format
    // ... fill frame data ...
    panelplayer_play_frame_bgr(frame, 128, 64);

    // Cleanup
    panelplayer_cleanup();
    return 0;
}
```

Compile:
```bash
gcc -lpanelplayer myapp.c -o myapp
```

### C# Wrapper

A full-featured C# wrapper is included in `examples/csharp/`:

```csharp
using var player = new PanelPlayer("eth0", 128, 64, 255);
player.SetMix(30);
player.PlayFile("animation.webp");
```

See `examples/csharp/README.md` for detailed usage.

## Extensions

Extensions allow custom frame processing without modifying PanelPlayer. They are dynamically loaded shared libraries.

### Creating an Extension

Minimal extension with `update` function:

```c
// my_extension.c
#include <stdint.h>

void update(int width, int height, uint8_t *frame) {
    // Modify frame data (BGR format, 3 bytes per pixel)
    for (int i = 0; i < width * height * 3; i += 3) {
        uint8_t blue = frame[i];
        uint8_t green = frame[i + 1];
        uint8_t red = frame[i + 2];

        // Your processing here
        uint8_t gray = (red + green + blue) / 3;
        frame[i] = frame[i + 1] = frame[i + 2] = gray;
    }
}
```

Optional `init` and `destroy` functions:

```c
#include <stdbool.h>
#include <stdio.h>

bool init() {
    printf("Extension initialized\n");
    return false;  // Return true on error
}

void destroy() {
    printf("Extension cleanup\n");
}

void update(int width, int height, uint8_t *frame) {
    // Process frame
}
```

Build extension:
```bash
gcc -shared -fPIC my_extension.c -o my_extension.so
```

Use extension:
```bash
panelplayer -p eth0 -w 128 -h 64 -e my_extension.so video.webp
```

Example extensions are in the `extensions/` directory:
- **grayscale**: Converts frames to grayscale
- **nanoled**: Custom LED matrix processing

## Architecture

```
┌─────────────────────────────────────────────────┐
│                   main.c                     │
│         (CLI argument parsing, file queue)   │
└────────────────┬────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────┐
│                  core.c                      │
│   (Timing, frame processing, playback loop)  │
└────┬───────────────────────────────────┬────────┘
     │                                 │
┌────▼───────────┐              ┌────────▼──────────┐
│   decoder.c   │              │  colorlight.c    │
│ (Multi-format │              │ (Ethernet proto- │
│  decoding)    │              │  col & sending)  │
└────────────────┘              └───────────────────┘
```

### Key Components

- **main.c**: CLI interface, file queue, shuffle mode
- **core.c**: Shared logic (timing, frame processing, playback)
- **decoder.c**: Multi-format image decoder (WebP/JPEG/PNG/GIF/BMP)
- **colorlight.c**: Ethernet protocol implementation
- **loader.c**: Background file loading with queue
- **panelplayer_api.c**: Library API wrapper

## Protocol

The Colorlight protocol uses raw ethernet frames (EtherType `0x0101`) with custom packet types:

| Type | Description |
|------|-------------|
| 0x01 | Display update (trigger screen refresh) |
| 0x0A | Set brightness/color balance |
| 0x55 | Image data (row-by-row transmission) |

Protocol documentation and Wireshark dissector are in the `protocol/` directory.

## Hardware Requirements

- **LED Panel**: Colorlight 5A-75B or compatible receiving card
- **Connection**: Direct ethernet connection (raw socket access required)
- **Permissions**: Root/sudo access for raw ethernet packets
- **Platform**: Linux (tested on Debian/Ubuntu, Orange Pi)

## Troubleshooting

### Permission Denied
```bash
# PanelPlayer requires raw socket access
sudo panelplayer -p eth0 -w 128 -h 64 image.jpg

# Or use capabilities (no sudo needed after setup)
sudo setcap cap_net_raw+ep /usr/local/bin/panelplayer
```

### Library Not Found
```bash
# Update library cache after installation
sudo ldconfig

# Or set LD_LIBRARY_PATH temporarily
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

### Wrong Ethernet Interface
```bash
# List network interfaces
ip link show

# Use the correct interface name
panelplayer -p <correct_interface> -w 128 -h 64 image.jpg
```

