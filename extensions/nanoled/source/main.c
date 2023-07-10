#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nanoled.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define SQUARE(a) ((a) * (a))

#define PORT "/dev/ttyACM0"
#define PIN 0
#define RGBW 1
#define DISTANCE 4
#define BLEND 0.9
#define TIMEOUT 4

nanoled *instance;
uint8_t buffer[4096];
int checksum = -1;
int attempts;

void mix(uint8_t *a, uint8_t b, float amount)
{
	*a = *a * amount + b * (1 - amount);
}

int set(int index, int centreX, int centreY, int width, uint8_t *frame)
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
	int w = MIN(MIN(r, g), b);

	r -= w;
	g -= w;
	b -= w;
#endif

	mix(buffer + index++, g, BLEND);
	mix(buffer + index++, r, BLEND);
	mix(buffer + index++, b, BLEND);

#if RGBW
	mix(buffer + index++, w, BLEND);
#endif

	return index;
}

bool init()
{
	instance = nanoled_init(PORT);
	return instance == NULL;
}

void update(int width, int height, uint8_t *frame)
{
	if (nanoled_read(instance) != checksum)
	{
		if (++attempts < TIMEOUT)
		{
			return;
		}

		puts("Failed to get response!");
	}

	attempts = 0;
	int index = 0;

	for (float x = DISTANCE; x < width - DISTANCE; x += (1000.0 / 60.0 / 2.0))
	{
		index = set(index, x, DISTANCE, width, frame);
	}

	checksum = nanoled_write(instance, PIN, buffer, index);
}

void destroy()
{
	if (instance != NULL)
	{
		nanoled_destroy(instance);
	}
}