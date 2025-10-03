#include <dlfcn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "core.h"

long core_get_time(void)
{
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

void core_await(long time)
{
	long delay = time - core_get_time();

	if (delay > 0)
	{
		usleep(delay * 1000);
	}
}

void core_process_frame(uint8_t *buffer, uint8_t *decoded, decoder_info *info,
                        int width, int height, int mix, bool initial)
{
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int source = (y * info->canvas_width + x) * 4;
			int destination = (y * width + x) * 3;

			int oldFactor = initial ? 0 : mix;
			int newFactor = CORE_MIX_MAXIMUM - oldFactor;

			buffer[destination] = (buffer[destination] * oldFactor + decoded[source + 2] * newFactor) / CORE_MIX_MAXIMUM;
			buffer[destination + 1] = (buffer[destination + 1] * oldFactor + decoded[source + 1] * newFactor) / CORE_MIX_MAXIMUM;
			buffer[destination + 2] = (buffer[destination + 2] * oldFactor + decoded[source] * newFactor) / CORE_MIX_MAXIMUM;
		}
	}
}

void core_send_frame(colorlight *cl, uint8_t *buffer, int width, int height,
                     bool duplicate, void (*update_func)(int, int, uint8_t*))
{
	if (update_func != NULL)
	{
		update_func(width, height, buffer);
	}

	for (int y = 0; y < height; y++)
	{
		colorlight_send_row(cl, y, width, buffer + y * width * 3);

		if (duplicate)
		{
			colorlight_send_row(cl, y + height, width, buffer + y * width * 3);
		}
	}
}

int core_load_extension(const char *path, void **extension, void (**update_func)(int, int, uint8_t*))
{
	if (path == NULL || extension == NULL || update_func == NULL)
	{
		return -1;
	}

	*extension = dlopen(path, RTLD_NOW);

	if (*extension == NULL)
	{
		return -1;
	}

	bool (*init)() = dlsym(*extension, "init");

	if (init != NULL)
	{
		if (init())
		{
			dlclose(*extension);
			*extension = NULL;
			return -1;
		}
	}

	*update_func = dlsym(*extension, "update");

	if (*update_func == NULL)
	{
		void (*destroy)() = dlsym(*extension, "destroy");
		if (destroy != NULL)
		{
			destroy();
		}
		dlclose(*extension);
		*extension = NULL;
		return -1;
	}

	return 0;
}

void core_unload_extension(void *extension)
{
	if (extension != NULL)
	{
		void (*destroy)() = dlsym(extension, "destroy");

		if (destroy != NULL)
		{
			destroy();
		}

		dlclose(extension);
	}
}
