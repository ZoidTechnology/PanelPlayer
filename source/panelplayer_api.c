#include <dlfcn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <webp/demux.h>

#include "panelplayer_api.h"
#include "colorlight.h"
#include "loader.h"

#define MIX_MAXIMUM 100
#define UPDATE_DELAY 10

typedef struct {
    colorlight *colorlight;
    uint8_t *buffer;
    int width;
    int height;
    int brightness;
    int mix;
    int rate;
    void *extension;
    void (*update_func)(int width, int height, uint8_t *frame);
    bool initialized;
} panelplayer_state;

static panelplayer_state g_state = {0};

static long get_time(void) {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

static void await(long time) {
    long delay = time - get_time();
    if (delay > 0) {
        usleep(delay * 1000);
    }
}

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
    g_state.extension = NULL;
    g_state.update_func = NULL;
    g_state.initialized = true;
    
    return PANELPLAYER_SUCCESS;
}

int panelplayer_set_mix(int mix_percentage) {
    if (!g_state.initialized) {
        return PANELPLAYER_NOT_INITIALIZED;
    }
    
    if (mix_percentage < 0 || mix_percentage >= MIX_MAXIMUM) {
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

int panelplayer_load_extension(const char* extension_path) {
    if (!g_state.initialized) {
        return PANELPLAYER_NOT_INITIALIZED;
    }
    
    if (!extension_path) {
        return PANELPLAYER_INVALID_PARAM;
    }
    
    if (g_state.extension) {
        void (*destroy)() = dlsym(g_state.extension, "destroy");
        if (destroy) {
            destroy();
        }
        dlclose(g_state.extension);
        g_state.extension = NULL;
        g_state.update_func = NULL;
    }
    
    g_state.extension = dlopen(extension_path, RTLD_NOW);
    if (!g_state.extension) {
        return PANELPLAYER_ERROR;
    }
    
    bool (*init)() = dlsym(g_state.extension, "init");
    if (init && init()) {
        dlclose(g_state.extension);
        g_state.extension = NULL;
        return PANELPLAYER_ERROR;
    }
    
    g_state.update_func = dlsym(g_state.extension, "update");
    if (!g_state.update_func) {
        dlclose(g_state.extension);
        g_state.extension = NULL;
        return PANELPLAYER_ERROR;
    }
    
    return PANELPLAYER_SUCCESS;
}

int panelplayer_play_webp_file(const char* file_path) {
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
    
    WebPData data = {
        .bytes = file_data,
        .size = file_size
    };
    
    WebPAnimDecoder *decoder = WebPAnimDecoderNew(&data, NULL);
    if (!decoder) {
        free(file_data);
        return PANELPLAYER_ERROR;
    }
    
    WebPAnimInfo info;
    WebPAnimDecoderGetInfo(decoder, &info);
    
    if (info.canvas_width < g_state.width || info.canvas_height < g_state.height) {
        WebPAnimDecoderDelete(decoder);
        free(file_data);
        return PANELPLAYER_ERROR;
    }
    
    long next = get_time();
    int previous = 0;
    bool initial = true;
    
    while (WebPAnimDecoderHasMoreFrames(decoder)) {
        uint8_t *decoded;
        int timestamp;
        
        WebPAnimDecoderGetNext(decoder, &decoded, &timestamp);
        
        for (int y = 0; y < g_state.height; y++) {
            for (int x = 0; x < g_state.width; x++) {
                int source = (y * info.canvas_width + x) * 4;
                int destination = (y * g_state.width + x) * 3;
                
                int oldFactor = initial ? 0 : g_state.mix;
                int newFactor = MIX_MAXIMUM - oldFactor;
                
                g_state.buffer[destination] = (g_state.buffer[destination] * oldFactor + decoded[source + 2] * newFactor) / MIX_MAXIMUM;
                g_state.buffer[destination + 1] = (g_state.buffer[destination + 1] * oldFactor + decoded[source + 1] * newFactor) / MIX_MAXIMUM;
                g_state.buffer[destination + 2] = (g_state.buffer[destination + 2] * oldFactor + decoded[source] * newFactor) / MIX_MAXIMUM;
            }
        }
        
        if (g_state.update_func) {
            g_state.update_func(g_state.width, g_state.height, g_state.buffer);
        }
        
        for (int y = 0; y < g_state.height; y++) {
            colorlight_send_row(g_state.colorlight, y, g_state.width, g_state.buffer + y * g_state.width * 3);
        }
        
        if (next - get_time() < UPDATE_DELAY) {
            next = get_time() + UPDATE_DELAY;
        }
        
        await(next);
        colorlight_send_update(g_state.colorlight, g_state.brightness, g_state.brightness, g_state.brightness);
        
        if (g_state.rate > 0) {
            next = get_time() + 1000 / g_state.rate;
        } else {
            next = get_time() + timestamp - previous;
            previous = timestamp;
        }
        
        initial = false;
    }
    
    WebPAnimDecoderDelete(decoder);
    free(file_data);
    
    return PANELPLAYER_SUCCESS;
}

int panelplayer_play_frame_bgr(const uint8_t* bgr_data, int width, int height) {
    if (!g_state.initialized) {
        return PANELPLAYER_NOT_INITIALIZED;
    }
    
    if (!bgr_data || width != g_state.width || height != g_state.height) {
        return PANELPLAYER_INVALID_PARAM;
    }
    
    memcpy(g_state.buffer, bgr_data, width * height * 3);
    
    if (g_state.update_func) {
        g_state.update_func(g_state.width, g_state.height, g_state.buffer);
    }
    
    for (int y = 0; y < g_state.height; y++) {
        colorlight_send_row(g_state.colorlight, y, g_state.width, g_state.buffer + y * g_state.width * 3);
    }
    
    long next = get_time() + UPDATE_DELAY;
    await(next);
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
    
    if (g_state.extension) {
        void (*destroy)() = dlsym(g_state.extension, "destroy");
        if (destroy) {
            destroy();
        }
        dlclose(g_state.extension);
    }
    
    if (g_state.colorlight) {
        colorlight_destroy(g_state.colorlight);
    }
    
    if (g_state.buffer) {
        free(g_state.buffer);
    }
    
    memset(&g_state, 0, sizeof(g_state));
}