#include <dlfcn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "panelplayer_api.h"
#include "colorlight.h"
#include "core.h"
#include "decoder.h"
#include "loader.h"

typedef struct {
    colorlight *colorlight;
    uint8_t *buffer;
    int width;
    int height;
    int brightness;
    int mix;
    int rate;
    bool duplicate;
    void *extension;
    void (*update_func)(int width, int height, uint8_t *frame);
    bool initialized;
} panelplayer_state;

static panelplayer_state g_state = {0};

int panelplayer_init(const char* interface_name, int width, int height, int brightness) {
    if (g_state.initialized) {
        return PANELPLAYER_ALREADY_INITIALIZED;
    }
    
    if (!interface_name || width <= 0 || height <= 0 || brightness < 0 || brightness > 255) {
        return PANELPLAYER_INVALID_PARAM;
    }
    
    g_state.colorlight = colorlight_init((char*)interface_name);
    if (!g_state.colorlight) {
        return PANELPLAYER_ERROR;
    }
    
    g_state.buffer = malloc(width * height * 3);
    if (!g_state.buffer) {
        colorlight_destroy(g_state.colorlight);
        return PANELPLAYER_ERROR;
    }
    
    g_state.width = width;
    g_state.height = height;
    g_state.brightness = brightness;
    g_state.mix = 0;
    g_state.rate = 0;
    g_state.duplicate = false;
    g_state.extension = NULL;
    g_state.update_func = NULL;
    g_state.initialized = true;
    
    return PANELPLAYER_SUCCESS;
}

int panelplayer_set_mix(int mix_percentage) {
    if (!g_state.initialized) {
        return PANELPLAYER_NOT_INITIALIZED;
    }

    if (mix_percentage < 0 || mix_percentage >= CORE_MIX_MAXIMUM) {
        return PANELPLAYER_INVALID_PARAM;
    }

    g_state.mix = mix_percentage;
    return PANELPLAYER_SUCCESS;
}

int panelplayer_set_rate(int frame_rate) {
    if (!g_state.initialized) {
        return PANELPLAYER_NOT_INITIALIZED;
    }

    if (frame_rate < 0) {
        return PANELPLAYER_INVALID_PARAM;
    }

    g_state.rate = frame_rate;
    return PANELPLAYER_SUCCESS;
}

int panelplayer_set_duplicate(bool enable) {
    if (!g_state.initialized) {
        return PANELPLAYER_NOT_INITIALIZED;
    }

    g_state.duplicate = enable;
    return PANELPLAYER_SUCCESS;
}

int panelplayer_load_extension(const char* extension_path) {
    if (!g_state.initialized) {
        return PANELPLAYER_NOT_INITIALIZED;
    }

    if (!extension_path) {
        return PANELPLAYER_INVALID_PARAM;
    }

    if (g_state.extension) {
        core_unload_extension(g_state.extension);
        g_state.extension = NULL;
        g_state.update_func = NULL;
    }

    if (core_load_extension(extension_path, &g_state.extension, &g_state.update_func) != 0) {
        return PANELPLAYER_ERROR;
    }

    return PANELPLAYER_SUCCESS;
}

int panelplayer_play_file(const char* file_path) {
    if (!g_state.initialized) {
        return PANELPLAYER_NOT_INITIALIZED;
    }

    if (!file_path) {
        return PANELPLAYER_INVALID_PARAM;
    }

    FILE *file = fopen(file_path, "rb");
    if (!file) {
        return PANELPLAYER_ERROR;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t *file_data = malloc(file_size);
    if (!file_data) {
        fclose(file);
        return PANELPLAYER_ERROR;
    }

    if (fread(file_data, 1, file_size, file) != file_size) {
        free(file_data);
        fclose(file);
        return PANELPLAYER_ERROR;
    }
    fclose(file);

    decoder *dec = decoder_init(file_data, file_size);
    if (!dec) {
        free(file_data);
        return PANELPLAYER_ERROR;
    }

    int result = core_play_decoded_file(dec, g_state.colorlight, g_state.buffer,
                                        g_state.width, g_state.height, g_state.brightness,
                                        g_state.mix, g_state.rate, g_state.duplicate,
                                        g_state.update_func);

    decoder_destroy(dec);
    free(file_data);

    return (result == 0) ? PANELPLAYER_SUCCESS : PANELPLAYER_ERROR;
}

int panelplayer_play_frame_bgr(const uint8_t* bgr_data, int width, int height) {
    if (!g_state.initialized) {
        return PANELPLAYER_NOT_INITIALIZED;
    }

    if (!bgr_data || width != g_state.width || height != g_state.height) {
        return PANELPLAYER_INVALID_PARAM;
    }

    memcpy(g_state.buffer, bgr_data, width * height * 3);

    core_send_frame(g_state.colorlight, g_state.buffer, g_state.width, g_state.height, g_state.duplicate, g_state.update_func);

    long next = core_get_time() + CORE_UPDATE_DELAY;
    core_await(next);
    colorlight_send_update(g_state.colorlight, g_state.brightness, g_state.brightness, g_state.brightness);

    return PANELPLAYER_SUCCESS;
}

int panelplayer_send_brightness(uint8_t red, uint8_t green, uint8_t blue) {
    if (!g_state.initialized) {
        return PANELPLAYER_NOT_INITIALIZED;
    }
    
    colorlight_send_brightness(g_state.colorlight, red, green, blue);
    return PANELPLAYER_SUCCESS;
}

bool panelplayer_is_initialized(void) {
    return g_state.initialized;
}

void panelplayer_cleanup(void) {
    if (!g_state.initialized) {
        return;
    }

    core_unload_extension(g_state.extension);

    if (g_state.colorlight) {
        colorlight_destroy(g_state.colorlight);
    }

    if (g_state.buffer) {
        free(g_state.buffer);
    }

    memset(&g_state, 0, sizeof(g_state));
}