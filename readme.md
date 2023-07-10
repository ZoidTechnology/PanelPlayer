# PanelPlayer
A [WebP](https://developers.google.com/speed/webp) player for Colorlight receiving cards. Tested with the Colorlight 5A-75B.

## Usage
PanelPlayer can be launched with `panelplayer <options> <sources>` where `<sources>` is one or more WebP files. The available options are:

### `-p <ethernet port>`
Sets which ethernet port to use for sending. This option is required.

### `-w <display width>`
Set the display width in pixels. This option is required.

### `-h <display height>`
Set the display height in pixels. This option is required.

### `-b <display brightness>`
Set the display brightness between 0 and 255. A value of 255 will be used if not specified.

### `-m <mix percentage>`
Controls the percentage of the previous frame to be blended with the current frame. Frame blending is disabled when set to 0 or not specified.

### `-r <frame rate>`
Overrides the source frame rate if specified.

### `-e <extension path>`
Load an extension from the path given. Only a single extension can be loaded.

### `-s`
Play sources randomly instead of in a fixed order. If used with a single source, this option will loop playback.

### `-v`
Enable verbose output.

## Building
Ensure `libwebp` is installed. PanelPlayer can be built by running `make` from within the root directory.

## Extensions
Extensions are a way to read or alter frames without modifying PanelPlayer. A minimal extension consists of an `update` function which gets called before each frame is sent. An extension may also include `init` and `destroy` functions. The `destroy` function will always be called if present, even when the `init` function indicates an error has occurred. Example extensions are located in the `extensions` directory.

## Protocol
Protocol documentation can be found in the `protocol` directory. A Wireshark plugin is included to help with reverse engineering and debugging.