/**
 * @file panelplayer_api.h
 * @brief PanelPlayer C API for controlling Colorlight LED panels
 * 
 * This header provides a C library interface for the PanelPlayer application,
 * allowing programmatic control of Colorlight LED receiving cards via raw ethernet.
 */

#ifndef PANELPLAYER_API_H
#define PANELPLAYER_API_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup ReturnCodes Return Codes
 * @{
 */
#define PANELPLAYER_SUCCESS 0                /**< Operation completed successfully */
#define PANELPLAYER_ERROR -1                 /**< General error occurred */
#define PANELPLAYER_INVALID_PARAM -2         /**< Invalid parameter provided */
#define PANELPLAYER_NOT_INITIALIZED -3       /**< Library not initialized */
#define PANELPLAYER_ALREADY_INITIALIZED -4   /**< Library already initialized */
/** @} */

typedef struct panelplayer_instance panelplayer_instance;

/**
 * @brief Initialize the PanelPlayer library
 * 
 * Initializes the PanelPlayer library with the specified LED panel parameters.
 * This function must be called before any other API functions.
 * 
 * @param interface_name Network interface name (e.g., "eth0")
 * @param width Panel width in pixels
 * @param height Panel height in pixels
 * @param brightness Initial brightness value (0-255)
 * 
 * @return PANELPLAYER_SUCCESS on success, error code otherwise
 * @retval PANELPLAYER_SUCCESS Initialization successful
 * @retval PANELPLAYER_INVALID_PARAM Invalid parameters provided
 * @retval PANELPLAYER_ALREADY_INITIALIZED Library already initialized
 * @retval PANELPLAYER_ERROR Failed to initialize network interface or allocate memory
 */
int panelplayer_init(const char* interface_name, int width, int height, int brightness);

/**
 * @brief Set frame mixing percentage
 * 
 * Controls how new frames are blended with existing content on the panel.
 * A value of 0 completely replaces the current frame, while higher values
 * blend the new frame with the existing content.
 * 
 * @param mix_percentage Mixing percentage (0-99)
 * 
 * @return PANELPLAYER_SUCCESS on success, error code otherwise
 * @retval PANELPLAYER_SUCCESS Mix percentage set successfully
 * @retval PANELPLAYER_NOT_INITIALIZED Library not initialized
 * @retval PANELPLAYER_INVALID_PARAM Mix percentage out of range
 */
int panelplayer_set_mix(int mix_percentage);

/**
 * @brief Set playback frame rate
 *
 * Sets the frame rate for WebP animation playback. When set to 0,
 * uses the timing information embedded in the WebP file.
 *
 * @param frame_rate Target frame rate in frames per second (0 for auto)
 *
 * @return PANELPLAYER_SUCCESS on success, error code otherwise
 * @retval PANELPLAYER_SUCCESS Frame rate set successfully
 * @retval PANELPLAYER_NOT_INITIALIZED Library not initialized
 * @retval PANELPLAYER_INVALID_PARAM Negative frame rate provided
 */
int panelplayer_set_rate(int frame_rate);

/**
 * @brief Enable or disable vertical duplication mode
 *
 * When enabled, each row of the image is sent twice - once at its original
 * position (y) and once at y+height. This is useful for driving two identical
 * displays stacked vertically where the image height is half the total display height.
 *
 * Example: With a 240x80 image and duplicate enabled, each row is sent to both
 * y and y+80, effectively filling a 240x160 display.
 *
 * @param enable true to enable duplication, false to disable
 *
 * @return PANELPLAYER_SUCCESS on success, error code otherwise
 * @retval PANELPLAYER_SUCCESS Duplicate mode set successfully
 * @retval PANELPLAYER_NOT_INITIALIZED Library not initialized
 */
int panelplayer_set_duplicate(bool enable);

/**
 * @brief Load a frame processing extension
 * 
 * Loads a shared library extension that can modify frame data before
 * it is sent to the LED panel. Extensions must implement the required
 * interface with update() function.
 * 
 * @param extension_path Path to the shared library (.so file)
 * 
 * @return PANELPLAYER_SUCCESS on success, error code otherwise
 * @retval PANELPLAYER_SUCCESS Extension loaded successfully
 * @retval PANELPLAYER_NOT_INITIALIZED Library not initialized
 * @retval PANELPLAYER_INVALID_PARAM NULL extension path provided
 * @retval PANELPLAYER_ERROR Failed to load extension or missing required functions
 */
int panelplayer_load_extension(const char* extension_path);

/**
 * @brief Play an image or animation file
 *
 * Loads and plays an image or animation file on the LED panel.
 * Supports WebP, JPEG, PNG, GIF, and BMP formats.
 * The function handles frame timing and loops through all frames for animated formats.
 *
 * @param file_path Path to the image file to play
 *
 * @return PANELPLAYER_SUCCESS on success, error code otherwise
 * @retval PANELPLAYER_SUCCESS File played successfully
 * @retval PANELPLAYER_NOT_INITIALIZED Library not initialized
 * @retval PANELPLAYER_INVALID_PARAM NULL file path provided
 * @retval PANELPLAYER_ERROR Failed to open file, decode image, or incompatible dimensions
 */
int panelplayer_play_file(const char* file_path);

/**
 * @brief Display a single frame from BGR data
 * 
 * Displays a single frame on the LED panel using raw BGR pixel data.
 * The data must be in BGR format with 3 bytes per pixel.
 * 
 * @param bgr_data Pointer to BGR pixel data (Blue, Green, Red order)
 * @param width Frame width in pixels (must match initialized width)
 * @param height Frame height in pixels (must match initialized height)
 * 
 * @return PANELPLAYER_SUCCESS on success, error code otherwise
 * @retval PANELPLAYER_SUCCESS Frame displayed successfully
 * @retval PANELPLAYER_NOT_INITIALIZED Library not initialized
 * @retval PANELPLAYER_INVALID_PARAM NULL data pointer or mismatched dimensions
 */
int panelplayer_play_frame_bgr(const uint8_t* bgr_data, int width, int height);

/**
 * @brief Send brightness/color balance command
 * 
 * Sends a brightness and color balance command directly to the LED panel.
 * This allows independent control of red, green, and blue channel brightness.
 * 
 * @param red Red channel brightness (0-255)
 * @param green Green channel brightness (0-255)
 * @param blue Blue channel brightness (0-255)
 * 
 * @return PANELPLAYER_SUCCESS on success, error code otherwise
 * @retval PANELPLAYER_SUCCESS Brightness command sent successfully
 * @retval PANELPLAYER_NOT_INITIALIZED Library not initialized
 */
int panelplayer_send_brightness(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Check if library is initialized
 * 
 * Returns the current initialization state of the PanelPlayer library.
 * 
 * @return true if library is initialized, false otherwise
 */
bool panelplayer_is_initialized(void);

/**
 * @brief Cleanup and shutdown the library
 * 
 * Cleans up all resources, closes network connections, unloads extensions,
 * and resets the library to an uninitialized state. Should be called
 * when finished using the library.
 */
void panelplayer_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif