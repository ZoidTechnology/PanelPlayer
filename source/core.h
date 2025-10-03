#ifndef CORE_H
#define CORE_H

#include <stdbool.h>
#include <stdint.h>

#include "colorlight.h"
#include "decoder.h"

#define CORE_MIX_MAXIMUM 100
#define CORE_UPDATE_DELAY 10

/**
 * Get current time in milliseconds
 */
long core_get_time(void);

/**
 * Sleep until the specified time (in milliseconds)
 */
void core_await(long time);

/**
 * Process a decoded frame: convert RGBA to BGR and apply frame mixing
 *
 * @param buffer Destination BGR buffer (width * height * 3)
 * @param decoded Source RGBA data (canvas_width * canvas_height * 4)
 * @param info Decoder info containing canvas dimensions
 * @param width Target width
 * @param height Target height
 * @param mix Mix percentage (0-99, 0 = no mixing)
 * @param initial True if this is the first frame (disables mixing)
 */
void core_process_frame(uint8_t *buffer, uint8_t *decoded, decoder_info *info,
                        int width, int height, int mix, bool initial);

/**
 * Send a frame to the hardware, applying extensions and duplicate mode
 *
 * @param cl Colorlight instance
 * @param buffer BGR frame data (width * height * 3)
 * @param width Frame width
 * @param height Frame height
 * @param duplicate If true, duplicate each row vertically
 * @param update_func Optional extension update function (can be NULL)
 */
void core_send_frame(colorlight *cl, uint8_t *buffer, int width, int height,
                     bool duplicate, void (*update_func)(int, int, uint8_t*));

/**
 * Load an extension from a shared library file
 *
 * @param path Path to the extension .so file
 * @param extension Output pointer to store the loaded extension handle
 * @param update_func Output pointer to store the update function pointer
 * @return 0 on success, -1 on error
 */
int core_load_extension(const char *path, void **extension, void (**update_func)(int, int, uint8_t*));

/**
 * Unload an extension, calling its destroy function if present
 *
 * @param extension Extension handle to unload (can be NULL)
 */
void core_unload_extension(void *extension);

/**
 * Play a decoded file through the complete playback loop
 *
 * @param dec Decoder instance with the loaded file
 * @param cl Colorlight instance for hardware output
 * @param buffer Frame buffer (width * height * 3 bytes)
 * @param width Display width
 * @param height Display height
 * @param brightness Brightness level (0-255)
 * @param mix Frame mixing percentage (0-99)
 * @param rate Frame rate override (0 = use source timing)
 * @param duplicate Enable vertical duplication mode
 * @param update_func Optional extension update function (can be NULL)
 * @return 0 on success, -1 on error
 */
int core_play_decoded_file(decoder *dec, colorlight *cl, uint8_t *buffer,
                           int width, int height, int brightness, int mix,
                           int rate, bool duplicate, void (*update_func)(int, int, uint8_t*));

#endif
