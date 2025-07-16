# PanelPlayer C# Example

This directory contains a C# example application that demonstrates how to use the PanelPlayer library from .NET applications.

## Prerequisites

- .NET 9.0 or later
- libpanelplayer.so compiled and available
- Root privileges (required for raw ethernet access)

## Building

### 1. Build the PanelPlayer Library

First, you need to build the shared library from the main project directory:

```bash
cd /path/to/PanelPlayer
make library
```

This will create `build/libpanelplayer.so`.

### 2. Copy Library to Output Directory

The C# project is configured to automatically copy the library to the output directory during build, but you can also copy it manually:

```bash
cp ../../build/libpanelplayer.so ./bin/Debug/net9.0/
```

### 3. Build the C# Application

```bash
dotnet build
```

## Running

The application must be run as root due to raw ethernet socket requirements:

```bash
sudo dotnet run [webp-file]
```

### Examples

Play a WebP file:
```bash
sudo dotnet run animation.webp
```

Run without arguments to see the blue frame demonstration:
```bash
sudo dotnet run
```

## Publish

Make a self contained app
```bash
dotnet publish -r linux-arm64 --self-contained true -c Release
```
## Usage

The example demonstrates:

1. **Basic initialization** - Setting up the panel with network interface and dimensions
2. **WebP file playback** - Playing WebP files (if provided as argument)
3. **Manual frame display** - Sending raw BGR pixel data to create a blue frame
4. **Proper cleanup** - Using `using` statement for automatic resource disposal

## Configuration

The example is configured for:
- Network interface: `end0` (eth0 is more common)
- Panel dimensions: 192x64 pixels
- Brightness: 255 (maximum)

Modify these values in `PanelPlayerExample.cs` to match your setup.

## Tested Environment

This example has been tested on:
- **Hardware**: Orange Pi Zero 3
- **OS**: Debian Bookworm
- **Runtime**: .NET 9.0.302

## Troubleshooting

### Permission Errors
Make sure you're running with `sudo` - raw ethernet access requires root privileges.

### Library Not Found
Ensure `libpanelplayer.so` is in the output directory or in your system's library path.

### Network Interface Issues
Verify that the network interface name (`end0`) matches your system's ethernet interface.

## API Reference

The C# wrapper provides these main methods:

- `PanelPlayer(interface, width, height, brightness)` - Initialize the panel
- `PlayWebPFile(path)` - Play animated WebP file
- `PlayFrameBGR(data, width, height)` - Display raw BGR frame data
- `SetBrightness(red, green, blue)` - Adjust color balance
- `LoadExtension(path)` - Load processing extensions
- `SetMix(percentage)` - Control frame blending
- `SetFrameRate(fps)` - Override animation timing

For detailed API documentation, see the native library header file `source/panelplayer_api.h`.