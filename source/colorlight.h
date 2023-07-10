#ifndef COLORLIGHT_H
#define COLORLIGHT_H

#include <stdint.h>

typedef struct colorlight colorlight;

colorlight *colorlight_init(char *interfaceName);
void colorlight_send_row(colorlight *instance, uint16_t row, uint16_t width, uint8_t *data);
void colorlight_send_update(colorlight *instance, uint8_t red, uint8_t green, uint8_t blue);
void colorlight_send_brightness(colorlight *instance, uint8_t red, uint8_t green, uint8_t blue);
void colorlight_destroy(colorlight *instance);

#endif