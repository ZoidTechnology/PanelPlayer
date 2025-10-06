#include <dlfcn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "colorlight.h"
#include "core.h"
#include "decoder.h"
#include "loader.h"

#define QUEUE_SIZE 4

bool parse(const char *source, int *destination)
{
	char *end;
	*destination = strtol(source, &end, 10);
	return end[0] != 0;
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
	bool duplicate = false;
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

			case 'd':
				duplicate = true;
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
			puts("  -d              Duplicate each row vertically");

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

	if (mix < 0 || mix >= CORE_MIX_MAXIMUM)
	{
		printf("Mix must be an integer between 0 and %d!\n", CORE_MIX_MAXIMUM - 1);
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
	void (*update)(int, int, uint8_t*) = NULL;

	if (extensionFile != NULL)
	{
		if (core_load_extension(extensionFile, &extension, &update) != 0)
		{
			puts("Failed to load extension!");
			goto destroy_colorlight;
		}
	}

	int queued = 0;

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

		decoder *decoder;

		if ((decoder = decoder_init(file, size)) == NULL)
		{
			puts("Failed to decode file!");
			goto free_file;
		}

		decoder_info info;
		decoder_get_info(decoder, &info);

		if (verbose)
		{
			printf("Decoding %d frames at a resolution of %dx%d.\n", info.frame_count, info.canvas_width, info.canvas_height);
		}

		long start = core_get_time();

		if (core_play_decoded_file(decoder, colorlight, buffer, width, height, brightness, mix, rate, duplicate, update) != 0)
		{
			puts("Image is smaller than display!");
			goto delete_decoder;
		}

		if (verbose)
		{
			long end = core_get_time();
			float seconds = (end - start) / 1000.0;
			printf("Played %d frames in %.2f seconds at an average rate of %.2f frames per second.\n", info.frame_count, seconds, info.frame_count / seconds);
		}

	delete_decoder:
		decoder_destroy(decoder);

	free_file:
		free(file);
	}

	status = EXIT_SUCCESS;

	core_unload_extension(extension);

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