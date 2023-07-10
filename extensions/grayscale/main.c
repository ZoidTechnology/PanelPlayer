#include <stdint.h>

void update(int width, int height, uint8_t *frame)
{
	for (int index = 0; index < width * height; index += 3)
	{
		float brightness = 0;

		brightness += frame[index] * 0.0722;
		brightness += frame[index + 1] * 0.7152;
		brightness += frame[index + 2] * 0.2126;
		
		frame[index] = brightness;
		frame[index + 1] = brightness;
		frame[index + 2] = brightness;
	}
}