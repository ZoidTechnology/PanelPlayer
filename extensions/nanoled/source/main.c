#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nanoled.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define SQUARE(a) ((a) * (a))

#define PORT "/dev/ttyACM0" // NanoLED serial port
#define PIN 0 // NanoLED output pin
#define LEDS 28 // Number of LEDs per side
#define PITCH (1000.0 / 60.0 / 2.0) // LED pitch in display pixels
#define EDGE ((256.0 - PITCH * LEDS) / 2.0) // Number of display pixels at either side of LED strip
#define RGBW true // Whether LED strip includes white LEDs
#define WHITE false // Use white LED instead of mixing red, green, and blue
#define DISTANCE 3 // Distance from centre to sample pixels
#define BLEND 0.75 // Amount of previous value to blend with current value
#define TIMEOUT 4 // Number of frames to wait for a valid response before sending another frame

nanoled *instance;
uint8_t buffer[4096];
int checksum = -1;
int attempts;

void mix(uint8_t *a, uint8_t b, float amount)
{
	*a = *a * amount + b * (1 - amount);
}

void set(int *index, int centreX, int centreY, int width, uint8_t *frame)
{
	int r = 0;
	int g = 0;
	int b = 0;

	for (int y = centreY - DISTANCE; y <= centreY + DISTANCE; y++)
	{
		for (int x = centreX - DISTANCE; x <= centreX + DISTANCE; x++)
		{
			int pixel = (y * width + x) * 3;

			r += SQUARE(frame[pixel + 2]);
			g += SQUARE(frame[pixel + 1]);
			b += SQUARE(frame[pixel]);
		}
	}

	int count = SQUARE(DISTANCE * 2 + 1);

	r /= count * 255;
	g /= count * 255;
	b /= count * 255;

#if RGBW
	int w = 0;

#if WHITE
	w = MIN(MIN(r, g), b);
	r -= w;
	g -= w;
	b -= w;
#endif
#endif

	mix(buffer + (*index)++, g, BLEND);
	mix(buffer + (*index)++, r, BLEND);
	mix(buffer + (*index)++, b, BLEND);

#if RGBW
	mix(buffer + (*index)++, w, BLEND);
#endif
}

bool init()
{
	instance = nanoled_init(PORT);
	return instance == NULL;
}

void update(int width, int height, uint8_t *frame)
{
	int index = 0;

	for (int led = 0; led < LEDS; led++)
	{
		set(&index, width - EDGE - (led + 0.5) * PITCH, DISTANCE, width, frame);
	}

	for (int led = 0; led < LEDS; led++)
	{
		set(&index, DISTANCE, EDGE + (led + 0.5) * PITCH, width, frame);
	}

	for (int led = 0; led < LEDS; led++)
	{
		set(&index, EDGE + (led + 0.5) * PITCH, height - DISTANCE - 1, width, frame);
	}

	for (int led = 0; led < LEDS; led++)
	{
		set(&index, width - DISTANCE - 1, height - EDGE - (led + 0.5) * PITCH, width, frame);
	}

	if (nanoled_read(instance) != checksum)
	{
		if (++attempts < TIMEOUT)
		{
			return;
		}

		puts("Failed to get response from NanoLED!");
	}

	attempts = 0;
	checksum = nanoled_write(instance, PIN, buffer, index);
}

void destroy()
{
	if (instance != NULL)
	{
		nanoled_destroy(instance);
	}
}