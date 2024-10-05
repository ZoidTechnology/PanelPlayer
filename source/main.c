#include <dlfcn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <webp/demux.h>

#include "colorlight.h"
#include "loader.h"

#define QUEUE_SIZE 4
#define MIX_MAXIMUM 100

bool parse(const char *source, int *destination)
{
	char *end;
	*destination = strtol(source, &end, 10);
	return end[0] != 0;
}

long get_time()
{
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

void await(long time)
{
	long delay = time - get_time();

	if (delay > 0)
	{
		usleep(delay * 1000);
	}
}

int main(int argc, char *argv[])
{
	int status = EXIT_FAILURE;
	char *port = NULL;
	int width = 0;
	int height = 0;
	int brightness = 255;
	int mix = 0;
	int rate = 0;
	char *extensionFile = NULL;
	bool shuffle = false;
	bool verbose = false;
	int sourcesLength = 0;
	char **sources;

	srand(time(NULL));

	if ((sources = malloc((argc - 1) * sizeof(*sources))) == NULL)
	{
		perror("Failed to allocate memory for sources");
		goto exit;
	}

	for (int index = 1; index < argc; index++)
	{
		char *argument = argv[index];

		if (argument[0] != '-')
		{
			sources[sourcesLength++] = argument;
			continue;
		}

		bool failed = false;

		switch (argument[1])
		{
			case 'p':
				failed = ++index >= argc;
				port = argv[index];
				break;

			case 'w':
				failed = ++index >= argc || parse(argv[index], &width);
				break;

			case 'h':
				failed = ++index >= argc || parse(argv[index], &height);
				break;

			case 'b':
				failed = ++index >= argc || parse(argv[index], &brightness);
				break;

			case 'm':
				failed = ++index >= argc || parse(argv[index], &mix);
				break;

			case 'r':
				failed = ++index >= argc || parse(argv[index], &rate);
				break;

			case 'e':
				failed = ++index >= argc;
				extensionFile = argv[index];
				break;

			case 's':
				shuffle = true;
				break;

			case 'v':
				verbose = true;
				break;

			default:
				failed = true;
		}

		if (failed || argument[2] != 0)
		{
			puts("Usage:");
			puts("  panelplayer -p <port> -w <width> -h <height> [options] <sources>");
			puts("");
			puts("Options:");
			puts("  -p <port>       Set ethernet port");
			puts("  -w <width>      Set display width");
			puts("  -h <height>     Set display height");
			puts("  -b <brightness> Set display brightness");
			puts("  -m <mix>        Set frame mixing percentage");
			puts("  -r <rate>       Override source frame rate");
			puts("  -e <extension>  Load extension from file");
			puts("  -s              Shuffle sources");
			puts("  -v              Enable verbose output");

			goto free_sources;
		}
	}

	if (port == NULL)
	{
		puts("Port must be specified!");
		goto free_sources;
	}

	if (width < 1 || height < 1)
	{
		puts("Width and height must be specified as positive integers!");
		goto free_sources;
	}

	if (brightness < 0 || brightness > 255)
	{
		puts("Brightness must be an integer between 0 and 255!");
		goto free_sources;
	}

	if (mix < 0 || mix >= MIX_MAXIMUM)
	{
		printf("Mix must be an integer between 0 and %d!\n", MIX_MAXIMUM - 1);
		goto free_sources;
	}

	if (sourcesLength == 0)
	{
		puts("At least one source must be specified!");
		goto free_sources;
	}

	uint8_t *buffer;

	if ((buffer = malloc(width * height * 3)) == NULL)
	{
		perror("Failed to allocate frame buffer");
		goto free_sources;
	}

	loader *loader;

	if ((loader = loader_init(QUEUE_SIZE)) == NULL)
	{
		puts("Failed to create loader instance!");
		goto free_buffer;
	}

	colorlight *colorlight;

	if ((colorlight = colorlight_init(port)) == NULL)
	{
		puts("Failed to create Colorlight instance!");
		goto destroy_loader;
	}

	void *extension = NULL;
	void (*update)() = NULL;

	if (extensionFile != NULL)
	{
		extension = dlopen(extensionFile, RTLD_NOW);

		if (extension == NULL)
		{
			puts("Failed to load extension!");
			goto destroy_colorlight;
		}

		bool (*init)() = dlsym(extension, "init");

		if (init != NULL)
		{
			if (init())
			{
				puts("Failed to initialise extension!");
				goto destroy_extension;
			}
		}

		update = dlsym(extension, "update");

		if (update == NULL)
		{
			puts("Extension does not provide update function!");
			goto destroy_extension;
		}
	}

	int queued = 0;
	bool initial = true;

	for (int source = 0; shuffle || source < sourcesLength; source++)
	{
		while (queued < source + QUEUE_SIZE)
		{
			if (shuffle)
			{
				loader_add(loader, sources[rand() % sourcesLength]);
			}
			else if (queued < sourcesLength)
			{
				loader_add(loader, sources[queued]);
			}

			queued++;
		}

		int size;
		void *file;

		if ((file = loader_get(loader, &size)) == NULL)
		{
			continue;
		}

		WebPData data = {
			.bytes = file,
			.size = size
		};

		WebPAnimDecoder *decoder;

		if ((decoder = WebPAnimDecoderNew(&data, NULL)) == NULL)
		{
			puts("Failed to decode file!");
			goto free_file;
		}

		WebPAnimInfo info;
		WebPAnimDecoderGetInfo(decoder, &info);

		if (verbose)
		{
			printf("Decoding %d frames at a resolution of %dx%d.\n", info.frame_count, info.canvas_width, info.canvas_height);
		}

		if (info.canvas_width < width || info.canvas_height < height)
		{
			puts("Image is smaller than display!");
			goto delete_decoder;
		}

		long start = 0;
		long next = 0;

		while (WebPAnimDecoderHasMoreFrames(decoder))
		{
			uint8_t *decoded;
			int timestamp;

			WebPAnimDecoderGetNext(decoder, &decoded, &timestamp);

			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < width; x++)
				{
					int source = (y * info.canvas_width + x) * 4;
					int destination = (y * width + x) * 3;

					int oldFactor = initial ? 0 : mix;
					int newFactor = MIX_MAXIMUM - oldFactor;

					buffer[destination] = (buffer[destination] * oldFactor + decoded[source + 2] * newFactor) / MIX_MAXIMUM;
					buffer[destination + 1] = (buffer[destination + 1] * oldFactor + decoded[source + 1] * newFactor) / MIX_MAXIMUM;
					buffer[destination + 2] = (buffer[destination + 2] * oldFactor + decoded[source] * newFactor) / MIX_MAXIMUM;
				}
			}

			if (update != NULL)
			{
				update(width, height, buffer);
			}

			for (int y = 0; y < height; y++)
			{
				colorlight_send_row(colorlight, y, width, buffer + y * width * 3);
			}

			if (start == 0)
			{
				start = next = get_time();
			}

			await(next);
			colorlight_send_update(colorlight, brightness, brightness, brightness);

			if (rate > 0)
			{
				next += 1000 / rate;
			}
			else
			{
				next = start + timestamp;
			}

			initial = false;
		}

		await(next);

		if (verbose)
		{
			float seconds = (get_time() - start) / 1000.0;
			printf("Played %d frames in %.2f seconds at an average rate of %.2f frames per second.\n", info.frame_count, seconds, info.frame_count / seconds);
		}

	delete_decoder:
		WebPAnimDecoderDelete(decoder);

	free_file:
		free(file);
	}

	status = EXIT_SUCCESS;

destroy_extension:
	if (extension != NULL)
	{
		void (*destroy)() = dlsym(extension, "destroy");

		if (destroy != NULL)
		{
			destroy();
		}

		dlclose(extension);
	}

destroy_colorlight:
	colorlight_destroy(colorlight);

destroy_loader:
	loader_destroy(loader);

free_buffer:
	free(buffer);

free_sources:
	free(sources);

exit:
	return status;
}