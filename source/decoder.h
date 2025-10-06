/**
 * @file decoder.h
 * @brief Multi-format image decoder abstraction for PanelPlayer
 *
 * This header provides a unified interface for decoding multiple image formats
 * including WebP, JPEG, PNG, GIF, and BMP. The decoder automatically detects
 * the format based on file headers and provides a consistent API regardless
 * of the underlying format.
 *
 * Supported formats:
 * - WebP (animated and static)
 * - JPEG (static)
 * - PNG (static)
 * - GIF (animated)
 * - BMP (static, 24/32-bit uncompressed)
 */

#ifndef DECODER_H
#define DECODER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Opaque decoder instance structure
 *
 * Internal structure that holds format-specific decoder state.
 * Users should treat this as an opaque handle.
 */
typedef struct decoder decoder;

/**
 * @brief Image/animation information structure
 *
 * Contains metadata about the decoded image or animation.
 */
typedef struct decoder_info
{
	int frame_count;    /**< Total number of frames (1 for static images) */
	int canvas_width;   /**< Image width in pixels */
	int canvas_height;  /**< Image height in pixels */
} decoder_info;

/**
 * @brief Initialize a decoder from memory data
 *
 * Creates a new decoder instance by analyzing the provided image data.
 * The format is automatically detected from the file header/magic bytes.
 *
 * The decoder supports:
 * - WebP: Animated and static images
 * - JPEG: Static images (24-bit RGB)
 * - PNG: Static images with alpha channel
 * - GIF: Animated images with transparency
 * - BMP: Static images (24/32-bit uncompressed)
 *
 * @param data Pointer to image file data in memory
 * @param size Size of the image data in bytes
 *
 * @return Decoder instance on success, NULL on failure
 * @retval NULL if format is unsupported, data is corrupted, or memory allocation fails
 *
 * @note The caller retains ownership of the data buffer and must ensure it
 *       remains valid for the lifetime of the decoder instance.
 *
 * @see decoder_destroy()
 */
decoder *decoder_init(void *data, int size);

/**
 * @brief Get information about the decoded image/animation
 *
 * Retrieves metadata including dimensions and frame count from the decoder.
 * This should be called after successful initialization to determine the
 * image properties before processing frames.
 *
 * @param instance Decoder instance
 * @param info Pointer to decoder_info structure to be filled
 *
 * @return true on success, false on failure
 * @retval true Information retrieved successfully
 * @retval false NULL instance or info pointer provided
 *
 * @note For static images (JPEG, PNG, BMP), frame_count will be 1.
 *       For animations (WebP, GIF), frame_count reflects the total frames.
 */
bool decoder_get_info(decoder *instance, decoder_info *info);

/**
 * @brief Check if more frames are available
 *
 * Determines whether there are additional frames to decode.
 * For static images, this returns true only before the first frame is read.
 * For animations, this returns true until all frames have been decoded.
 *
 * @param instance Decoder instance
 *
 * @return true if more frames available, false otherwise
 * @retval true More frames can be decoded
 * @retval false No more frames or NULL instance
 *
 * @see decoder_get_next()
 */
bool decoder_has_more_frames(decoder *instance);

/**
 * @brief Decode and retrieve the next frame
 *
 * Decodes the next available frame from the image/animation.
 * Returns a pointer to the frame data in RGBA format (4 bytes per pixel)
 * and the timestamp for animation timing.
 *
 * Frame data format:
 * - 4 bytes per pixel: Red, Green, Blue, Alpha
 * - Row-major order (left-to-right, top-to-bottom)
 * - Size: canvas_width × canvas_height × 4 bytes
 *
 * Timestamp behavior:
 * - Static images: Always 0
 * - GIF: Cumulative time in milliseconds from start
 * - WebP: Cumulative time in milliseconds from start
 *
 * @param instance Decoder instance
 * @param frame Output pointer to frame data (RGBA format)
 * @param timestamp Output pointer to frame timestamp in milliseconds
 *
 * @return true on success, false on failure
 * @retval true Frame decoded successfully
 * @retval false No more frames, NULL parameters, or decode error
 *
 * @warning The frame pointer is valid only until the next call to
 *          decoder_get_next() or decoder_destroy(). Do not free this pointer.
 *
 * @see decoder_has_more_frames()
 */
bool decoder_get_next(decoder *instance, uint8_t **frame, int *timestamp);

/**
 * @brief Destroy decoder and free resources
 *
 * Releases all resources associated with the decoder instance including
 * internal buffers and format-specific decoder state. The decoder instance
 * becomes invalid after this call.
 *
 * @param instance Decoder instance to destroy (can be NULL)
 *
 * @note Safe to call with NULL instance (no-op).
 * @note Does not free the original data buffer passed to decoder_init().
 *
 * @see decoder_init()
 */
void decoder_destroy(decoder *instance);

#endif
