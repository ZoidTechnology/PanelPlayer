# PanelPlayer C# Example

This directory contains a C# example application that demonstrates how to use the PanelPlayer library from .NET applications.

## Prerequisites

- .NET 8.0 or later (supports both .NET 8.0 and 9.0)
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

The application must be run as root due to raw ethernet socket requirements. It uses command-line arguments similar to the native PanelPlayer:

```bash
sudo dotnet run -- -p <port> -w <width> -h <height> [options] <sources>
```

### Options

- `-p <port>` - Set ethernet port (required, e.g., eth0, end0)
- `-w <width>` - Set display width in pixels (required)
- `-h <height>` - Set display height in pixels (required)
- `-b <brightness>` - Set display brightness (0-255, default: 255)
- `-m <mix>` - Set frame mixing percentage (0-99, default: 0)
- `-r <rate>` - Override source frame rate
- `-e <extension>` - Load extension from file
- `-d` - Duplicate each row vertically
- `-v` - Enable verbose output

### Examples

Play a single image file:
```bash
sudo dotnet run -- -p eth0 -w 192 -h 64 image.jpg
```

Play multiple animation files with verbose output:
```bash
sudo dotnet run -- -p eth0 -w 192 -h 64 -v animation1.webp animation2.gif
```

Play with frame mixing and custom brightness:
```bash
sudo dotnet run -- -p eth0 -w 192 -h 64 -b 200 -m 50 video.webp
```

Play with duplicate mode (for stacked displays):
```bash
sudo dotnet run -- -p eth0 -w 192 -h 64 -d content.png
```

Load an extension:
```bash
sudo dotnet run -- -p eth0 -w 192 -h 64 -e ../../extensions/grayscale/extension.so image.jpg
```

## Publish

Create a self-contained application:
```bash
dotnet publish -r linux-arm64 --self-contained true -c Release
```

After publishing, the executable will be in `bin/Release/net8.0/linux-arm64/publish/`:
```bash
sudo ./bin/Release/net8.0/linux-arm64/publish/PanelPlayerExample -p eth0 -w 192 -h 64 image.jpg
```

Note: The build creates binaries for both .NET 8.0 and 9.0. Use the appropriate version based on your runtime.

## Project Structure

- `Program.cs` - Main entry point with command-line argument parsing
- `PanelPlayer.cs` - Managed wrapper class for the native library
- `PanelPlayerNative.cs` - P/Invoke declarations for libpanelplayer.so

## Tested Environment

This example has been tested on:
- **Hardware**: Orange Pi Zero 3
- **OS**: Debian Bookworm
- **Runtime**: .NET 8.0 and .NET 9.0

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
- `PlayFile(path)` - Play image/animation file (WebP, JPEG, PNG, GIF, BMP)
- `PlayFrameBGR(data, width, height)` - Display raw BGR frame data
- `SetBrightness(red, green, blue)` - Adjust color balance
- `LoadExtension(path)` - Load processing extensions
- `SetMix(percentage)` - Control frame blending
- `SetFrameRate(fps)` - Override animation timing
- `SetDuplicate(enable)` - Enable vertical duplication for stacked displays

For detailed API documentation, see the native library header file `source/panelplayer_api.h`.